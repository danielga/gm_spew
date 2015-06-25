#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/Lua/LuaInterface.h>
#include <lua.hpp>
#include <dbg.h>
#include <color.h>
#include <sstream>
#include <queue>
#include <mutex>
#include <stdint.h>

#if defined _WIN32

	#include <Windows.h>

	DWORD mainthread;

#else

	#include <pthread.h>

	pthread_t mainthread;

#endif

GarrysMod::Lua::ILuaInterface *lua = nullptr;
SpewOutputFunc_t oldspew = nullptr;
volatile bool insidespew = false;

struct Spew
{
	Spew( SpewType_t type, int level, const char *group, const Color &color, const char *msg ) :
		type( type ),
		level( level ),
		group( group ),
		color( color ),
		msg( msg )
	{ }

	SpewType_t type;
	int level;
	std::string group;
	Color color;
	std::string msg;
};

std::queue<Spew> spew_queue;
std::mutex spew_locker;

LUA_FUNCTION_STATIC( HookRun )
{
	int args = LUA->Top( );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );
	LUA->GetField( -1, "hook" );
	LUA->GetField( -1, "Run" );

	for( int i = 1; i <= args; ++i )
		LUA->Push( i );

	LUA->Call( args, 1 );
	return 1;
}

LUA_FUNCTION_STATIC( ErrorTraceback )
{
	GarrysMod::Lua::ILuaInterface *lua = static_cast<GarrysMod::Lua::ILuaInterface *>( LUA );

	std::string spaces( 2, ' ' );
	std::ostringstream stream;
	stream << LUA->GetString( 1 ) << '\n';

	lua_Debug dbg = { 0 };
	for( int level = 1; lua->GetStack( level, &dbg ) == 1; ++level, memset( &dbg, 0, sizeof( dbg ) ) )
	{
		if( level != 1 )
			stream << '\n';

		lua->GetInfo( "Sln", &dbg );
		stream
			<< spaces
			<< level
			<< ". "
			<< ( dbg.name == nullptr ? "unknown" : dbg.name )
			<< " - "
			<< dbg.short_src
			<< ':'
			<< dbg.currentline;
		spaces += ' ';
	}

	LUA->PushString( stream.str( ).c_str( ) );
	return 1;
}

static bool SpewToLua( SpewType_t type, int level, const char *group, const Color &color, const char *msg )
{
	lua->PushCFunction( ErrorTraceback );
	lua->PushCFunction( HookRun );
	lua->PushString( "EngineSpew" );
	lua->PushNumber( type );
	lua->PushNumber( level );
	lua->PushString( group );
	lua->PushNumber( color[0] );
	lua->PushNumber( color[1] );
	lua->PushNumber( color[2] );
	lua->PushString( msg );

	bool dontcall = false;
	if( lua->PCall( 8, 1, -10 ) == 0 )
		dontcall = lua->IsType( -1, GarrysMod::Lua::Type::BOOL ) && !lua->GetBool( -1 );
	else
		lua->Msg( "\n[ERROR] %s\n\n", lua->GetString( -1 ) );

	lua->Pop( 1 );

	return dontcall;
}

static SpewRetval_t EngineSpewReceiver( SpewType_t type, const char *msg )
{

#if defined _WIN32

	if( GetCurrentThreadId( ) != mainthread )

#else

	if( pthread_self( ) != mainthread )

#endif

	{
		spew_locker.lock( );
		spew_queue.push(
			Spew(
				type,
				GetSpewOutputLevel( ),
				GetSpewOutputGroup( ),
				*GetSpewOutputColor( ),
				msg
			)
		);
		spew_locker.unlock( );

		return oldspew( type, msg );
	}

	if( insidespew )
		return oldspew( type, msg );

	insidespew = true;

	while( !spew_queue.empty( ) )
	{
		spew_locker.lock( );
		Spew spew = spew_queue.front( );
		spew_queue.pop( );
		spew_locker.unlock( );
		SpewToLua( spew.type, spew.level, spew.group.c_str( ), spew.color, spew.msg.c_str( ) );
	}

	bool dontcall = SpewToLua( type, GetSpewOutputLevel( ), GetSpewOutputGroup( ), *GetSpewOutputColor( ), msg );

	insidespew = false;

	return dontcall ? SPEW_CONTINUE : oldspew( type, msg );
}

GMOD_MODULE_OPEN( )
{
	lua = static_cast<GarrysMod::Lua::ILuaInterface *>( LUA );

	#if defined _WIN32

		mainthread = GetCurrentThreadId( );

	#else

		mainthread = pthread_self( );

	#endif

	oldspew = GetSpewOutputFunc( );
	SpewOutputFunc( EngineSpewReceiver );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushNumber( SPEW_MESSAGE );
	LUA->SetField( -2, "SPEW_MESSAGE" );

	LUA->PushNumber( SPEW_WARNING );
	LUA->SetField( -2, "SPEW_WARNING" );

	LUA->PushNumber( SPEW_ASSERT );
	LUA->SetField( -2, "SPEW_ASSERT" );

	LUA->PushNumber( SPEW_ERROR );
	LUA->SetField( -2, "SPEW_ERROR" );

	LUA->PushNumber( SPEW_LOG );
	LUA->SetField( -2, "SPEW_LOG" );

	return 0;
}

GMOD_MODULE_CLOSE( )
{
	SpewOutputFunc( oldspew );
	return 0;
}