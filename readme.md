# gm\_spew

[![Build Status](https://metamann.visualstudio.com/GitHub%20danielga/_apis/build/status/danielga.gm_spew?branchName=master)](https://metamann.visualstudio.com/GitHub%20danielga/_build/latest?definitionId=18&branchName=master)

A Garry's Mod module that intercepts the engine's messages and passes them to Lua. Also able to prevent them from printing.

## API reference

When return types or parameters are enclosed in square brackets, they are nillable returns or optional parameters and will contain the default value, in the case of parameters.

### Types

```lua
Message = { -- Contains a log message and all its metadata.
    time = number, -- Timestamp of the message.
    type = number, -- Represents the type of the message.
    level = number, -- Message level, interacts with the "developer" convar.
    group = string, -- Contains the message group name.
    color = { -- Message color.
        r = number, -- Amount of red of the message color.
        g = number, -- Amount of green of the message color.
        b = number, -- Amount of blue of the message color.
        a = number -- Amount of alpha of the message color.
    },
    message = string -- Contains the log message.
}

Messages = {Message} -- 'Messages' is an array of 'Message'
```

### Constants

```lua
-- Holds the 'spew' module version in a string form.
-- Example: "spew 1.0.0"
string spew.Version
```

```lua
-- Holds the 'spew' module version in a numeric form, LuaJIT style.
-- Example: 10000
number spew.VersionNum
```

```lua
-- Type enum for simple messages.
number spew.MESSAGE
```

```lua
-- Type enum for warning messages.
number spew.WARNING
```

```lua
-- Type enum for assert messages.
number spew.ASSERT
```

```lua
-- Type enum for error messages.
number spew.ERROR
```

```lua
-- Type enum for log messages.
number spew.LOG
```

### Functions

```lua
-- Get up to the specified number of messages from the queue.
-- NOTE: Avoid retrieving messages one by one in loops.
-- 'count' is a number, containing the number of messages to retrieve.
-- Returns an array of 'Message' if messages are available, otherwise returns 'nil'.
[Messages] spew.Get([count = 1])
```

```lua
-- Get all the messages in the queue.
-- Returns an array of 'Message' if messages are available, otherwise returns 'nil'.
[Messages] spew.GetAll()
```

```lua
-- Block a certain type of messages from being logged.
-- When -1 is passed, all message types are blocked.
-- Recommended to use the enums for message types, as their value can change anytime in the future.
-- 'message_type' is a number, representing the message type to be blocked.
-- Returns true if the operation was successful, false otherwise.
boolean spew.Block([message_type = -1])
```

```lua
-- Unblock a certain type of messages from being logged.
-- When -1 is passed, all message types are unblocked.
-- Recommended to use the enums for message types, as their value can change anytime in the future.
-- 'message_type' is a number, representing the message type to be unblocked.
-- Returns true if the operation was successful, false otherwise.
boolean spew.Unblock([message_type = -1])
```

```lua
-- Returns the blocked state of a certain type of messages.
-- Recommended to use the enums for message types, as their value can change anytime in the future.
-- 'message_type' is a number, representing the message type to get state of.
-- Returns true if the message type is blocked, false otherwise.
boolean spew.IsBlocked(message_type)
```

```lua
-- Sets the maximum log queue size.
-- After this amount of logs, older messages are dequeued.
-- 'max_queue_size' is a number, representing the maximum log queue size.
spew.SetMaximumQueueSize(max_queue_size)
```

```lua
-- Returns the maximum log queue size.
-- After this amount of logs, older messages are dequeued.
number spew.GetMaximumQueueSize()
```

These functions are only available when the Lua extension module `spewx` is required.

```lua
-- Enables the Lua hook "Spew".
-- spew.Get and spew.GetAll will be unreliable when this hook is enabled, as it uses these same functions to work, causing the log queue to be consumed.
-- Uses the "Think" hook to dispatch log messages, hence it will seem to be disabled when used in a dedicated server with no players. This limitation can be bypassed by setting the "sv_hibernate_think" convar to 1.
spew.EnableHook()
```

```lua
-- Disables the Lua hook "Spew".
spew.DisableHook()
```

### Hooks

This hook is only available when the Lua extension module `spewx` is required.

```lua
-- Hook called when a message is logged.
-- 'timestamp' is a number representing the time it was logged at.
-- 'type' is a number, equal to one of the type enums.
-- 'level' is a number, with which the "developer" convar interacts.
-- 'group' is a string, with the name of the message group.
-- 'message' is a string, containing the log message.
-- Example: hook.Add("Spew", "logger", function(...) end)
Spew(timestamp, type, level, group, color, message)
```

## Compiling

The only supported compilation platform for this project on Windows is **Visual Studio 2017**. However, it's possible it'll work with *Visual Studio 2015* and *Visual Studio 2019* because of the unified runtime.

On Linux, everything should work fine as is.

For macOS, any **Xcode (using the GCC compiler)** version *MIGHT* work as long as the **Mac OSX 10.7 SDK** is used.

These restrictions are not random; they exist because of ABI compatibility reasons.

If stuff starts erroring or fails to work, be sure to check the correct line endings (`\n` and such) are present in the files for each OS.

## Requirements

This project requires [garrysmod\_common][1], a framework to facilitate the creation of compilations files (Visual Studio, make, XCode, etc). Simply set the environment variable `GARRYSMOD_COMMON` or the premake option `--gmcommon=path` to the path of your local copy of [garrysmod\_common][1].

We also use [SourceSDK2013][2]. The links to [SourceSDK2013][2] point to my own fork of VALVe's repo and for good reason: Garry's Mod has lots of backwards incompatible changes to interfaces and it's much smaller, being perfect for automated build systems like Azure Pipelines.

  [1]: https://github.com/danielga/garrysmod_common
  [2]: https://github.com/danielga/sourcesdk-minimal
