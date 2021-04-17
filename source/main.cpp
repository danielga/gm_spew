#include <GarrysMod/Lua/Interface.h>

#include <dbg.h>
#include <Color.h>

#include <cstdint>
#include <queue>
#include <mutex>
#include <limits>

namespace spew
{

class Spew
{
public:
	Spew( SpewType_t type, int32_t level, const char *group, const Color &color, const char *msg ) :
		time( Plat_FloatTime( ) ),
		type( type ),
		level( level ),
		group( group ),
		color( color ),
		msg( msg )
	{ }

	void ToLua( GarrysMod::Lua::ILuaBase *LUA ) const
	{
		LUA->CreateTable( );

			LUA->PushNumber( time );
			LUA->SetField( -2, "time" );

			LUA->PushNumber( type );
			LUA->SetField( -2, "type" );

			LUA->PushNumber( level );
			LUA->SetField( -2, "level" );

			LUA->PushString( group.c_str( ) );
			LUA->SetField( -2, "group" );

			LUA->CreateTable( );

				LUA->PushNumber( color[0] );
				LUA->SetField( -2, "r" );

				LUA->PushNumber( color[1] );
				LUA->SetField( -2, "g" );

				LUA->PushNumber( color[2] );
				LUA->SetField( -2, "b" );

				LUA->PushNumber( color[3] );
				LUA->SetField( -2, "a" );

			LUA->SetField( -2, "color" );

			LUA->PushString( msg.c_str( ) );
			LUA->SetField( -2, "message" );
	}

private:
	double time;
	SpewType_t type;
	int32_t level;
	std::string group;
	Color color;
	std::string msg;
};

static size_t max_messages = 1000;
static SpewOutputFunc_t original_spew = nullptr;
static std::queue<Spew> spew_queue;
static std::mutex spew_locker;
static bool blocked_spews[SPEW_TYPE_COUNT] = { false, false, false, false, false };

static std::vector<Spew> PopMessages( const size_t num = -1 )
{
	size_t count = num;
	const size_t size = spew_queue.size( );
	if( count > size )
		count = size;

	std::vector<Spew> messages;
	if( count == 0 )
		return messages;

	messages.reserve( count );

	std::lock_guard<std::mutex> lock( spew_locker );
	for( size_t k = 0; k < count; ++k )
	{
		messages.emplace_back( spew_queue.front( ) );
		spew_queue.pop( );
	}

	return messages;
}

static int32_t PushMessages( GarrysMod::Lua::ILuaBase *LUA, const size_t num = -1 )
{
	const std::vector<Spew> messages = PopMessages( num );
	if( messages.empty( ) )
		return 0;

	LUA->CreateTable( );

	size_t k = 0;
	for( const auto &spew : messages )
	{
		LUA->PushNumber( ++k );

		spew.ToLua( LUA );

		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( Get )
{
	size_t count = 1;
	if( LUA->Top( ) != 0 )
		count = static_cast<size_t>( LUA->CheckNumber( 1 ) );

	return PushMessages( LUA, count );
}

LUA_FUNCTION_STATIC( GetAll )
{
	return PushMessages( LUA );
}

static int32_t SetBlockState( GarrysMod::Lua::ILuaBase *LUA, const bool enabled )
{
	int32_t type = -1;
	if( LUA->Top( ) != 0 )
		type = static_cast<int32_t>( LUA->CheckNumber( 1 ) );

	switch( type )
	{
		case -1:
			for( size_t k = 0; k < SPEW_TYPE_COUNT; ++k )
				blocked_spews[k] = enabled;

			LUA->PushBool( true );
			break;

		case SPEW_MESSAGE:
		case SPEW_WARNING:
		case SPEW_ASSERT:
		case SPEW_ERROR:
		case SPEW_LOG:
			blocked_spews[type] = enabled;
			LUA->PushBool( true );
			break;

		default:
			LUA->PushBool( false );
			break;
	}

	return 1;
}

LUA_FUNCTION_STATIC( Block )
{
	return SetBlockState( LUA, true );
}

LUA_FUNCTION_STATIC( Unblock )
{
	return SetBlockState( LUA, false );
}

LUA_FUNCTION_STATIC( IsBlocked )
{
	const int32_t type = static_cast<int32_t>( LUA->CheckNumber( 1 ) );
	if( type < SPEW_MESSAGE || type >= SPEW_TYPE_COUNT )
		LUA->ArgError( 1, "unknown spew type" );

	LUA->PushBool( blocked_spews[type] );
	return 1;
}

LUA_FUNCTION_STATIC( SetMaximumQueueSize )
{
	const double number = LUA->CheckNumber( 1 );
	if( number < std::numeric_limits<size_t>::min( ) || number > std::numeric_limits<size_t>::max( ) )
		LUA->ArgError( 1, "number is out of bounds" );

	max_messages = static_cast<size_t>( number );
	return 0;
}

LUA_FUNCTION_STATIC( GetMaximumQueueSize )
{
	LUA->PushNumber( static_cast<double>( max_messages ) );
	return 1;
}

static SpewRetval_t EngineSpewReceiver( SpewType_t type, const char *msg )
{
	{
		Spew message(
			type,
			GetSpewOutputLevel( ),
			GetSpewOutputGroup( ),
			*GetSpewOutputColor( ),
			msg
		);

		std::lock_guard<std::mutex> lock( spew_locker );
		if( spew_queue.size( ) >= max_messages )
			spew_queue.pop( );

		spew_queue.emplace( std::move( message ) );
	}

	if( type >= SPEW_MESSAGE && type < SPEW_TYPE_COUNT && blocked_spews[type] )
		return SPEW_CONTINUE;

	return original_spew( type, msg );
}

static void Initialize( GarrysMod::Lua::ILuaBase *LUA )
{
	original_spew = GetSpewOutputFunc( );
	SpewOutputFunc( EngineSpewReceiver );

	LUA->CreateTable( );

	LUA->PushString( "spew 1.1.0" );
	LUA->SetField( -2, "Version" );

	// version num follows LuaJIT style, xxyyzz
	LUA->PushNumber( 10100 );
	LUA->SetField( -2, "VersionNum" );

	LUA->PushNumber( SPEW_MESSAGE );
	LUA->SetField( -2, "MESSAGE" );

	LUA->PushNumber( SPEW_WARNING );
	LUA->SetField( -2, "WARNING" );

	LUA->PushNumber( SPEW_ASSERT );
	LUA->SetField( -2, "ASSERT" );

	LUA->PushNumber( SPEW_ERROR );
	LUA->SetField( -2, "ERROR" );

	LUA->PushNumber( SPEW_LOG );
	LUA->SetField( -2, "LOG" );

	LUA->PushCFunction( Get );
	LUA->SetField( -2, "Get" );

	LUA->PushCFunction( GetAll );
	LUA->SetField( -2, "GetAll" );

	LUA->PushCFunction( Block );
	LUA->SetField( -2, "Block" );

	LUA->PushCFunction( Unblock );
	LUA->SetField( -2, "Unblock" );

	LUA->PushCFunction( IsBlocked );
	LUA->SetField( -2, "IsBlocked" );

	LUA->PushCFunction( SetMaximumQueueSize );
	LUA->SetField( -2, "SetMaximumQueueSize" );

	LUA->PushCFunction( GetMaximumQueueSize );
	LUA->SetField( -2, "GetMaximumQueueSize" );

	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, "spew" );
}

static void Deinitialize( GarrysMod::Lua::ILuaBase *LUA )
{
	SpewOutputFunc( original_spew );

	LUA->PushNil( );
	LUA->SetField( GarrysMod::Lua::INDEX_GLOBAL, "spew" );
}

}

GMOD_MODULE_OPEN( )
{
	spew::Initialize( LUA );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	spew::Deinitialize( LUA );
	return 0;
}
