newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = _OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON")
if gmcommon == nil then
	error("you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
end

include(gmcommon)

CreateWorkspace({name = "spew"})
	CreateProject({serverside = true})
		IncludeSDKCommon()
		IncludeSDKTier0()

	CreateProject({serverside = false})
		IncludeSDKCommon()
		IncludeSDKTier0()
