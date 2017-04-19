#include <GarrysMod/Lua/Interface.h>
#include <dbg.h>
#include <color.h>
#include <cstdint>
#include <queue>
#include <mutex>

namespace spew
{

struct Spew
{
	Spew( ) :
		type( SPEW_MESSAGE ),
		level( -1 )
	{ }

	Spew( SpewType_t type, int level, const char *group, const Color &color, const char *msg ) :
		time( Plat_FloatTime( ) ),
		type( type ),
		level( level ),
		group( group ),
		color( color ),
		msg( msg )
	{ }

	double time;
	SpewType_t type;
	int32_t level;
	std::string group;
	Color color;
	std::string msg;
};

static const size_t MAX_MESSAGES = 100000;
static SpewOutputFunc_t original_spew = nullptr;
static std::queue<Spew> spew_queue;
static std::mutex spew_locker;
static bool blocked_spews[SPEW_TYPE_COUNT] = { false };

LUA_FUNCTION_STATIC( Get )
{
	size_t count = 1;
	if( LUA->Top( ) != 0 )
		count = static_cast<size_t>( LUA->CheckNumber( 1 ) );

	size_t size = spew_queue.size( );
	if( count > size )
		count = size;

	if( count == 0 )
		return 0;

	LUA->CreateTable( );

	Spew spew;
	for( size_t k = 0; k < count; ++k )
	{
		spew_locker.lock( );
		spew = spew_queue.front( );
		spew_queue.pop( );
		spew_locker.unlock( );

		LUA->PushNumber( k + 1 );
		LUA->CreateTable( );

		LUA->PushNumber( spew.time );
		LUA->SetField( -2, "time" );

		LUA->PushNumber( spew.type );
		LUA->SetField( -2, "type" );

		LUA->PushNumber( spew.level );
		LUA->SetField( -2, "message" );

		LUA->PushString( spew.group.c_str( ) );
		LUA->SetField( -2, "group" );

		LUA->CreateTable( );

		LUA->PushNumber( spew.color[0] );
		LUA->SetField( -2, "r" );

		LUA->PushNumber( spew.color[1] );
		LUA->SetField( -2, "g" );

		LUA->PushNumber( spew.color[2] );
		LUA->SetField( -2, "b" );

		LUA->PushNumber( spew.color[3] );
		LUA->SetField( -2, "a" );

		LUA->SetField( -2, "color" );

		LUA->PushString( spew.msg.c_str( ) );
		LUA->SetField( -2, "message" );

		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( Block )
{
	int32_t type = -1;
	if( LUA->Top( ) != 0 )
		type = static_cast<uint32_t>( LUA->CheckNumber( 1 ) );

	switch( type )
	{
	case -1:
		for( size_t k = 0; k < SPEW_TYPE_COUNT; ++k )
			blocked_spews[k] = true;

		break;

	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_ASSERT:
	case SPEW_ERROR:
	case SPEW_LOG:
		blocked_spews[type] = true;
		LUA->PushBool( true );
		break;

	default:
		LUA->PushBool( false );
		break;
	}

	return 1;
}

LUA_FUNCTION_STATIC( Unblock )
{
	int32_t type = -1;
	if( LUA->Top( ) != 0 )
		type = static_cast<uint32_t>( LUA->CheckNumber( 1 ) );

	switch( type )
	{
	case -1:
		for( size_t k = 0; k < SPEW_TYPE_COUNT; ++k )
			blocked_spews[k] = false;

		break;

	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_ASSERT:
	case SPEW_ERROR:
	case SPEW_LOG:
		blocked_spews[type] = false;
		LUA->PushBool( true );
		break;

	default:
		LUA->PushBool( false );
		break;
	}

	return 1;
}

static SpewRetval_t EngineSpewReceiver( SpewType_t type, const char *msg )
{
	spew_locker.lock( );
	if( spew_queue.size( ) >= MAX_MESSAGES )
		spew_queue.pop( );

	spew_queue.push( Spew(
		type,
		GetSpewOutputLevel( ),
		GetSpewOutputGroup( ),
		*GetSpewOutputColor( ),
		msg
	) );
	spew_locker.unlock( );

	return ( type < SPEW_TYPE_COUNT && blocked_spews[type] ) ? SPEW_CONTINUE : original_spew( type, msg );
}

static void Initialize( GarrysMod::Lua::ILuaBase *LUA )
{
	original_spew = GetSpewOutputFunc( );
	SpewOutputFunc( EngineSpewReceiver );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->CreateTable( );

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

	LUA->PushCFunction( spew::Get );
	LUA->SetField( -2, "Get" );

	LUA->PushCFunction( spew::Block );
	LUA->SetField( -2, "Block" );

	LUA->PushCFunction( spew::Unblock );
	LUA->SetField( -2, "Unblock" );

	LUA->SetField( -2, "spew" );

	LUA->Pop( 1 );
}

static void Deinitialize( GarrysMod::Lua::ILuaBase *LUA )
{
	SpewOutputFunc( original_spew );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushNil( );
	LUA->SetField( -2, "spew" );

	LUA->Pop( 1 );
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
