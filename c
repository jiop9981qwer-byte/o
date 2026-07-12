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

local WinLimitReached = false

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

-- [[ FUNCTIONS (100% RESTORED FROM ORIGINAL) ]]

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

local function TriggerSmartKill()
    _G.InRematchLoop = false
    if not _G.AutoKill then return end
    task.spawn(function()
        task.wait(6)
        KillAll()
        task.wait(3)
        KillAll()
    end)
end

local function StartAutoQueue()
    local MatchmakingRemotes = ReplicatedStorage:FindFirstChild("MatchmakingShared") and ReplicatedStorage.MatchmakingShared:FindFirstChild("Remotes")
    if MatchmakingRemotes then
        local activeModes = {}
        for mode, selected in pairs(_G.SelectedModes) do
            if selected then table.insert(activeModes, mode) end
        end
        if #activeModes > 0 then
            pcall(function()
                if MatchmakingRemotes:FindFirstChild("SetModes") then MatchmakingRemotes.SetModes:FireServer(activeModes) end
                task.wait(1)
                if MatchmakingRemotes:FindFirstChild("QueueModes") then 
                    MatchmakingRemotes.QueueModes:InvokeServer(activeModes) 
                end
            end)
        end
    end
end

-- [[ UI ELEMENTS ]]

CombatTab:CreateSection("Kill Functions")
CombatTab:CreateButton({
    Name = "Kill All (Instant)",
    Callback = function() KillAll() end,
})
CombatTab:CreateToggle({
    Name = "Auto Kill (6s + 3s)",
    CurrentValue = false,
    Flag = "AutoKill",
    Callback = function(Value) _G.AutoKill = Value end,
})

MatchmakingTab:CreateSection("Auto Rematch Settings")
MatchmakingTab:CreateDropdown({
	Name = "Win Limit (Server Hop)",
	Options = {"1", "2", "3", "4", "5", "6", "7", "8"},
	CurrentOption = "7",
	Flag = "WinLimit",
	Callback = function(Option)
		_G.RematchLimit = tonumber(Option)
	end,
})
MatchmakingTab:CreateToggle({
    Name = "Auto Rematch",
    CurrentValue = false,
    Flag = "AutoRematch",
    Callback = function(Value) _G.AutoRematch = Value end,
})

MatchmakingTab:CreateSection("Auto Queue (Lobby & Match)")
MatchmakingTab:CreateToggle({
    Name = "Auto Queue",
    CurrentValue = false,
    Flag = "AutoQueue",
    Callback = function(Value) 
        _G.AutoQueue = Value 
        if Value then StartAutoQueue() end
    end,
})

MatchmakingTab:CreateSection("Select Modes")
for _, mode in ipairs({"1v1", "2v2", "3v3", "4v4"}) do
    MatchmakingTab:CreateToggle({
        Name = "Mode: " .. mode,
        CurrentValue = false,
        Flag = "Mode_" .. mode,
        Callback = function(Value) _G.SelectedModes[mode] = Value end,
    })
end

MatchmakingTab:CreateSection("Misc")
MatchmakingTab:CreateToggle({
    Name = "Anti-AFK",
    CurrentValue = false,
    Flag = "AntiAFK",
    Callback = function(Value) _G.AntiAFK = Value end,
})

ConfigTab:CreateSection("Configuration Management")
local ConfigName = ""
ConfigTab:CreateInput({
	Name = "Config Name",
	PlaceholderText = "Enter name (e.g. farm)",
	RemoveTextAfterFocusLost = false,
	Callback = function(Text) ConfigName = Text end,
})
ConfigTab:CreateButton({
	Name = "Create & Save Config",
	Callback = function()
		if ConfigName ~= "" then
			Rayfield:SaveConfiguration(ConfigName)
			Rayfield:Notify({Title = "Config", Content = "Saved: " .. ConfigName})
		end
	end,
})
ConfigTab:CreateButton({
	Name = "Load Config",
	Callback = function()
		if ConfigName ~= "" then
			Rayfield:LoadConfiguration(ConfigName)
			Rayfield:Notify({Title = "Config", Content = "Loaded: " .. ConfigName})
		end
	end,
})

ConfigTab:CreateSection("Keybinds")
ConfigTab:CreateKeybind({
	Name = "Toggle UI",
	CurrentKeybind = "Backquote",
	HoldToInteract = false,
	Flag = "UIKeybind",
	Callback = function()
		Window:Toggle()
	end,
})

-- [[ CONSTANT REMATCH VOTE LOOP (5-10s) ]]

task.spawn(function()
    local MatchmakingRemotes = ReplicatedStorage:WaitForChild("MatchmakingShared", 10) and ReplicatedStorage.MatchmakingShared:WaitForChild("Remotes", 10)
    local RematchVote = MatchmakingRemotes and MatchmakingRemotes:FindFirstChild("RematchVote")
    
    while true do
        if _G.AutoRematch and not WinLimitReached then
            pcall(function()
                if RematchVote then RematchVote:FireServer() end
            end)
            task.wait(math.random(5, 10)) -- 5~10초 랜덤 간격
        else
            task.wait(1)
        end
    end
end)

-- [[ MAIN AUTOMATION LOGIC ]]

task.spawn(function()
    local Remotes = ReplicatedStorage:WaitForChild("Remotes", 10)
    local MatchmakingRemotes = ReplicatedStorage:WaitForChild("MatchmakingShared", 10) and ReplicatedStorage.MatchmakingShared:WaitForChild("Remotes", 10)
    
    -- 1. Game Start & Round Events (Original Restored)
    if Remotes then
        local RoundCleanup = Remotes:WaitForChild("RoundCleanup", 10)
        local ClientLoaded = Remotes:WaitForChild("ClientLoaded", 10)
        
        if RoundCleanup then RoundCleanup.OnClientEvent:Connect(function(matchId) if matchId and type(matchId) == "string" then _G.CurrentMatchId = matchId end WinLimitReached = false TriggerSmartKill() end) end
        if ClientLoaded then ClientLoaded.OnClientEvent:Connect(function(matchId) if matchId and type(matchId) == "string" then _G.CurrentMatchId = matchId end WinLimitReached = false TriggerSmartKill() end) end
    end

    -- 2. Matchmaking & Rematch Logic
    if MatchmakingRemotes then
        local PartyStateChanged = MatchmakingRemotes:WaitForChild("PartyStateChanged", 10)
        local RematchState = MatchmakingRemotes:WaitForChild("RematchState", 10)
        
        local function UpdateMatchStatus(data)
            if data and data.matched ~= nil then
                _G.IsMatched = data.matched
                if not _G.IsMatched and _G.AutoQueue and not WinLimitReached then StartAutoQueue() end
            end
        end

        if PartyStateChanged then PartyStateChanged.OnClientEvent:Connect(UpdateMatchStatus) end

        if RematchState then
            RematchState.OnClientEvent:Connect(function(data)
                if data then
                    -- 승리 제한 체크
                    local currentWinsA = data.winsA or 0
                    local currentWinsB = data.winsB or 0
                    
                    if currentWinsA >= _G.RematchLimit or currentWinsB >= _G.RematchLimit then
                        if not WinLimitReached then
                            WinLimitReached = true
                            Rayfield:Notify({Title = "Win Limit", Content = "Limit Reached! Switching Server..."})
                            
                            -- 강제 퀵 매치 실행 (StartAutoQueue 로직 사용)
                            task.spawn(function()
                                for i = 1, 3 do
                                    StartAutoQueue()
                                    task.wait(2)
                                end
                                ServerHop()
                            end)
                        end
                        return
                    end
                end
            end)
        end
    end
end)

-- Anti-AFK
LocalPlayer.Idled:Connect(function()
    if _G.AntiAFK then
        VirtualUser:CaptureController()
        VirtualUser:ClickButton2(Vector2.new())
    end
end)

-- Fallback Loop for Auto Queue
task.spawn(function()
    while true do
        if _G.AutoQueue and not _G.IsMatched and not WinLimitReached then
            StartAutoQueue()
        end
        task.wait(10)
    end
end)

task.spawn(function()
    task.wait(2)
    pcall(function() Rayfield:LoadConfiguration() end)
end)

Rayfield:Notify({
    Title = "Koji HUD",
    Content = "5-10s Rematch & Win Limit Queue Fixed!"
})
