require("spew")

local function PassMessagesToHook()
    local messages = spew.GetAll()
    if not messages then
        return
    end

    for i = 1, #messages do
        local msg = messages[i]
        hook.Run("Spew", msg.Timestamp, msg.Type, msg.Level, msg.Group, Color(msg.Color.r, msg.Color.g, msg.Color.b, msg.Color.a), msg.Message)
    end
end

function spew.EnableHook()
    hook.Add("Think", "spew.PassMessagesToHook", PassMessagesToHook)
end

function spew.DisableHook()
    hook.Remove("Think", "spew.PassMessagesToHook")
end
