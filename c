local Rayfield = loadstring(game:HttpGet('https://sirius.menu/rayfield'))()
local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local TeleportService = game:GetService("TeleportService")
local HttpService = game:GetService("HttpService")
local VirtualUser = game:GetService("VirtualUser")

local LocalPlayer = Players.LocalPlayer
while not LocalPlayer do task.wait(0.5) LocalPlayer = Players.LocalPlayer end

-- [[ GLOBAL STATES ]]
_G.AutoKill = false
_G.AutoQueue = false
_G.AutoRematch = false
_G.AntiAFK = false
_G.RematchLimit = 7
_G.CurrentGameNumber = 1
_G.CurrentMatchId = ""
_G.SelectedModes = {["1v1"] = false, ["2v2"] = false, ["3v3"] = false, ["4v4"] = false}
_G.IsMatched = false
_G.InCleanupWait = false

-- [[ AUTOMATIC DUEL STATES ]]
_G.TargetDuelUserId = 0
_G.AutoDuelChallenge = false
_G.AutoAcceptDuel = false

local WinLimitReached = false
local queueCoroutine = nil
local duelChallengeCoroutine = nil

-- [[ UI SETUP ]]
local Window = Rayfield:CreateWindow({
    Name = "Koji HUD",
    LoadingTitle = "Koji HUD Loading...",
    LoadingSubtitle = "by Koji",
    ConfigurationSaving = {
        Enabled = true,
        FolderName = "KojiHUD",
        FileName = "MurderDuels_" .. tostring(LocalPlayer.UserId)
    }
})

local CombatTab = Window:CreateTab("Combat", 4483362458)
local MatchmakingTab = Window:CreateTab("Matchmaking", 4483362458)
local ConfigTab = Window:CreateTab("Settings", 4483362458)

-- [[ FUNCTIONS ]]
local function ServerHop()
    local success = pcall(function()
        local servers = HttpService:JSONDecode(game:HttpGet("https://games.roblox.com/v1/games/" .. game.PlaceId .. "/servers/Public?sortOrder=Asc&limit=100"))
        for _, server in pairs(servers.data) do
            if server.playing < server.maxPlayers and server.id ~= game.JobId then
                TeleportService:TeleportToPlaceInstance(game.PlaceId, server.id, LocalPlayer)
                return true
            end
        end
    end)
    return success
end

local function ExecuteKill(target)
    local Remotes = ReplicatedStorage:FindFirstChild("Remotes")
    if not Remotes then return end
    local ThrowRemote = Remotes:FindFirstChild("ThrowReplicate")
    local HitRemote = Remotes:FindFirstChild("ReportHit")
    if not ThrowRemote or not HitRemote then return end

    if not target.Character or not target.Character:FindFirstChild("Head") then return end
    local char = target.Character
    local head = char.Head
    local myChar = LocalPlayer.Character
    if not myChar or not myChar:FindFirstChild("HumanoidRootPart") then return end
    
    local myPos = myChar.HumanoidRootPart.Position
    local targetPos = head.Position
    local currentId = math.random(1, 99999)
    
    pcall(function()
        ThrowRemote:FireServer({
            ["toolName"] = "Knife",
            ["id"] = currentId,
            ["ownerUserId"] = LocalPlayer.UserId,
            ["origin"] = myPos,
            ["isExplosive"] = false,
            ["power"] = 1,
            ["target"] = targetPos,
            ["effects"] = {Shotgun=0, Portal=0, Smoke=0, Explosive=0, Flammable=0}
        })
        
        HitRemote:FireServer({
            ["hitPos"] = targetPos,
            ["ownerUserId"] = LocalPlayer.UserId,
            ["origin"] = myPos,
            ["vel"] = Vector3.new(100, 100, 100),
            ["headshot"] = true,
            ["targetUserId"] = target.UserId,
            ["targetModel"] = char,
            ["to"] = targetPos,
            ["throwId"] = currentId,
            ["kind"] = "throw",
            ["at"] = tick(),
            ["hitPart"] = head
        })
    end)
end

local function KillAll()
    for _, p in ipairs(Players:GetPlayers()) do
        if p ~= LocalPlayer and p.Character and p.Character:FindFirstChild("Humanoid") then
            if p.Character.Humanoid.Health > 0 and not p.Character:FindFirstChildOfClass("ForceField") then
                pcall(function() ExecuteKill(p) end)
            end
        end
    end
end

local function OnRoundCleanup()
    if not _G.AutoQueue and not _G.AutoDuelChallenge then return end
    task.spawn(function()
        _G.InCleanupWait = true
        task.wait(38) -- 38초 대기
        _G.InCleanupWait = false
    end)
end

local function TriggerSmartKill()
    _G.InRematchLoop = false
    OnRoundCleanup()
    if not _G.AutoKill then return end
    task.spawn(function()
        task.wait(5)
        for i = 1, 3 do if not _G.AutoKill then break end KillAll() task.wait(0.33) end
        task.wait(2)
        for i = 1, 3 do if not _G.AutoKill then break end KillAll() task.wait(0.33) end
    end)
end

local function sendQueueSignal()
    local MatchmakingRemotes = ReplicatedStorage:FindFirstChild("MatchmakingShared") and ReplicatedStorage.MatchmakingShared:FindFirstChild("Remotes")
    if MatchmakingRemotes then
        local activeModes = {}
        for mode, selected in pairs(_G.SelectedModes) do if selected then table.insert(activeModes, mode) end end
        if #activeModes > 0 then
            pcall(function()
                if MatchmakingRemotes:FindFirstChild("SetModes") then MatchmakingRemotes.SetModes:FireServer(activeModes) end
                task.wait(1)
                if MatchmakingRemotes:FindFirstChild("QueueModes") then MatchmakingRemotes.QueueModes:InvokeServer(activeModes) end
            end)
        end
    end
end

-- [[ UI ELEMENTS ]]
CombatTab:CreateSection("Kill Functions")
CombatTab:CreateButton({ Name = "Kill All (Instant)", Callback = function() KillAll() end })
CombatTab:CreateToggle({ Name = "Auto Kill", CurrentValue = false, Flag = "AutoKill", Callback = function(Value) _G.AutoKill = Value end })

MatchmakingTab:CreateSection("Auto Rematch Settings")
MatchmakingTab:CreateDropdown({
    Name = "Win Limit (Server Hop)",
    Options = {"1", "2", "3", "4", "5", "6", "7", "8"},
    CurrentOption = "7",
    Flag = "WinLimit",
    Callback = function(Option) _G.RematchLimit = tonumber(Option) end,
})
MatchmakingTab:CreateToggle({ Name = "Auto Rematch", CurrentValue = false, Flag = "AutoRematch", Callback = function(Value) _G.AutoRematch = Value end })

MatchmakingTab:CreateSection("Auto Queue")
MatchmakingTab:CreateToggle({ Name = "Auto Queue", CurrentValue = false, Flag = "AutoQueue", Callback = function(Value) 
    _G.AutoQueue = Value 
    if Value then
        task.spawn(function()
            while _G.AutoQueue do
                if not _G.InCleanupWait and not _G.IsMatched and not WinLimitReached then sendQueueSignal() end
                task.wait(10)
            end
        end)
    end
end })

MatchmakingTab:CreateSection("Select Modes")
for _, mode in ipairs({"1v1", "2v2", "3v3", "4v4"}) do
    MatchmakingTab:CreateToggle({ Name = "Mode: " .. mode, CurrentValue = false, Flag = "Mode_" .. mode, Callback = function(Value) _G.SelectedModes[mode] = Value end })
end

MatchmakingTab:CreateSection("Automatic Duel System")
MatchmakingTab:CreateInput({ Name = "User ID", PlaceholderText = "Enter Target UserID...", Callback = function(Text) _G.TargetDuelUserId = tonumber(Text) or 0 end })
MatchmakingTab:CreateToggle({ Name = "Auto Duel", CurrentValue = false, Callback = function(Value)
    _G.AutoDuelChallenge = Value
    if Value then
        task.spawn(function()
            while _G.AutoDuelChallenge do
                local RequestSendRemote = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("PlayerRequestSend")
                if _G.TargetDuelUserId > 0 and RequestSendRemote and not _G.IsMatched and not _G.InCleanupWait then
                    pcall(function() RequestSendRemote:InvokeServer({Type = "Duel", TargetUserId = _G.TargetDuelUserId}) end)
                    task.wait(10) -- 10초 간격 시도
                else task.wait(2) end
            end
        end)
    end
end })

MatchmakingTab:CreateToggle({ Name = "Auto Accept", CurrentValue = false, Flag = "AutoAcceptDuel", Callback = function(Value) _G.AutoAcceptDuel = Value end })

MatchmakingTab:CreateSection("Misc")
MatchmakingTab:CreateToggle({ Name = "Anti-AFK", CurrentValue = false, Flag = "AntiAFK", Callback = function(Value) _G.AntiAFK = Value end })

ConfigTab:CreateSection("Configuration Management")
local ConfigName = ""
ConfigTab:CreateInput({ Name = "Config Name", PlaceholderText = "Enter name", RemoveTextAfterFocusLost = false, Callback = function(Text) ConfigName = Text end })
ConfigTab:CreateButton({ Name = "Save", Callback = function() if ConfigName ~= "" then Rayfield:SaveConfiguration(ConfigName) end end })
ConfigTab:CreateButton({ Name = "Load", Callback = function() if ConfigName ~= "" then Rayfield:LoadConfiguration(ConfigName) end end })

ConfigTab:CreateSection("Keybinds")
ConfigTab:CreateKeybind({ Name = "Toggle UI", CurrentKeybind = "Backquote", Callback = function() Window:Toggle() end })

-- [[ CONSTANT REMATCH VOTE LOOP ]]
task.spawn(function()
    local MatchmakingRemotes = ReplicatedStorage:WaitForChild("MatchmakingShared", 10) and ReplicatedStorage.MatchmakingShared:WaitForChild("Remotes", 10)
    local RematchVote = MatchmakingRemotes and MatchmakingRemotes:FindFirstChild("RematchVote")
    while true do
        if _G.AutoRematch and not WinLimitReached then
            pcall(function() if RematchVote then RematchVote:FireServer() end end)
            task.wait(math.random(5, 10))
        else task.wait(1) end
    end
end)

-- [[ MAIN AUTOMATION LOGIC ]]
task.spawn(function()
    local Remotes = ReplicatedStorage:WaitForChild("Remotes", 20)
    local MatchmakingRemotes = ReplicatedStorage:WaitForChild("MatchmakingShared", 20) and ReplicatedStorage.MatchmakingShared:WaitForChild("Remotes", 20)
    
    if Remotes then
        local RoundCleanup = Remotes:WaitForChild("RoundCleanup", 10)
        local ClientLoaded = Remotes:WaitForChild("ClientLoaded", 10)
        local RequestNotify = Remotes:WaitForChild("PlayerRequestNotify", 10)
        local RespondRemote = Remotes:WaitForChild("PlayerRequestRespond", 10)
        
        if RoundCleanup then RoundCleanup.OnClientEvent:Connect(function() WinLimitReached = false TriggerSmartKill() end) end
        if ClientLoaded then ClientLoaded.OnClientEvent:Connect(function() WinLimitReached = false TriggerSmartKill() end) end
        
        if RequestNotify and RespondRemote then
            RequestNotify.OnClientEvent:Connect(function(data)
                if not _G.AutoAcceptDuel then return end
                local requestId = nil
                if type(data) == "string" then requestId = data
                elseif type(data) == "table" then
                    requestId = data.Id or data.id or data.UUID or data.RequestId
                    if not requestId then
                        for _, v in pairs(data) do
                            if type(v) == "string" and #v == 36 and string.find(v, "-") then requestId = v break end
                        end
                    end
                end
                if requestId then
                    task.wait(0.25)
                    pcall(function() RespondRemote:FireServer(requestId, true) end)
                end
            end)
        end
    end

    if MatchmakingRemotes then
        local PartyStateChanged = MatchmakingRemotes:WaitForChild("PartyStateChanged", 10)
        local RematchState = MatchmakingRemotes:WaitForChild("RematchState", 10)
        if PartyStateChanged then PartyStateChanged.OnClientEvent:Connect(function(data) if data then _G.IsMatched = data.matched end end) end
        if RematchState then
            RematchState.OnClientEvent:Connect(function(data)
                if data and (data.winsA >= _G.RematchLimit or data.winsB >= _G.RematchLimit) then
                    WinLimitReached = true
                    task.spawn(function() for i=1,3 do sendQueueSignal() task.wait(2) end ServerHop() end)
                end
            end)
        end
    end
end)

LocalPlayer.Idled:Connect(function() if _G.AntiAFK then VirtualUser:CaptureController() VirtualUser:ClickButton2(Vector2.new()) end end)
task.spawn(function() task.wait(2) pcall(function() Rayfield:LoadConfiguration() end) end)

Rayfield:Notify({ Title = "Koji HUD", Content = "Systems Initialized" })
