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
_G.AutoStreakRestore = false
_G.RematchLimit = 7
_G.SelectedModes = {["1v1"] = false, ["2v2"] = false, ["3v3"] = false, ["4v4"] = false}
_G.IsMatched = false
_G.InCleanupWait = false
_G.AutoAcceptDuel = false 

-- [[ 릴레이 관련 전역 변수 ]]
_G.QueueList = {}          
_G.PendingRotation = false 

local killThread = nil
local WinLimitReached = false

-- [[ FILE STORAGE PATHS ]]
local ID_CONFIG_FILE = "KojiHUD_SavedIDs.json"
local CONFIG_FOLDER = "KojiHUD_Configs/"
local PERMANENT_AUTOSAVE_FILE = "KojiHUD_AutoSave.json"

if not isfolder(CONFIG_FOLDER) then makefolder(CONFIG_FOLDER) end

-- [[ ID STORAGE LOGIC ]]
local savedIDs = {}
local function loadSavedIDs()
    if isfile(ID_CONFIG_FILE) then
        local success, result = pcall(function() return HttpService:JSONDecode(readfile(ID_CONFIG_FILE)) end)
        if success and type(result) == "table" then 
            savedIDs = result 
            _G.QueueList = {}
            for _, v in ipairs(result) do
                table.insert(_G.QueueList, tonumber(v))
            end
        else 
            savedIDs = {} 
            _G.QueueList = {}
        end
    else 
        savedIDs = {} 
        _G.QueueList = {}
    end
end

local function saveID(idText)
    local idNum = tonumber(idText)
    if not idNum then return end
    for _, val in ipairs(savedIDs) do if val == tostring(idNum) then return end end
    table.insert(savedIDs, tostring(idNum))
    table.insert(_G.QueueList, idNum)
    writefile(ID_CONFIG_FILE, HttpService:JSONEncode(savedIDs))
end

local function deleteID(idText)
    if not idText or idText == "" then return end
    for i, val in ipairs(savedIDs) do
        if val == idText then table.remove(savedIDs, i) break end
    end
    local targetNum = tonumber(idText)
    for i, val in ipairs(_G.QueueList) do
        if val == targetNum then table.remove(_G.QueueList, i) break end
    end
    writefile(ID_CONFIG_FILE, HttpService:JSONEncode(savedIDs))
end

loadSavedIDs()

-- [[ RAYFIELD WINDOW SETUP ]]
local Window = Rayfield:CreateWindow({
    Name = "Koji HUD | by Koji",
    LoadingTitle = "Koji HUD Loading...",
    LoadingSubtitle = "Please wait",
    ConfigurationSaving = {
        Enabled = false
    },
    KeySystem = false
})

-- 탭 생성
local MainTab = Window:CreateTab("Main", "home")
local CombatTab = Window:CreateTab("Combat", "sword")
local MatchmakingTab = Window:CreateTab("Matchmaking", "user")
local MatchmakingDuelTab = Window:CreateTab("Matchmaking(Duel)", "users") 
local SettingsTab = Window:CreateTab("Settings", "settings")

local Elements = {}

-- [[ CONFIG & AUTOSAVE CORE LOGIC ]]
local function GetConfigList()
    local list = {}
    local files = isfolder(CONFIG_FOLDER) and listfiles(CONFIG_FOLDER) or {}
    for _, file in ipairs(files) do
        local name = file:match("([^/]+)%.json$") or file:match("([^\\]+)%.json$")
        if name then table.insert(list, name) end
    end
    if #list == 0 then table.insert(list, "None") end
    return list
end

local function GetCurrentSettingsTable()
    return {
        AutoKill = _G.AutoKill,
        AutoStreakRestore = _G.AutoStreakRestore,
        RematchLimit = tostring(_G.RematchLimit),
        AutoRematch = _G.AutoRematch,
        AutoQueue = _G.AutoQueue,
        SelectedModes = _G.SelectedModes,
        AutoDuel = _G.AutoDuelChallenge,
        AutoAccept = _G.AutoAcceptDuel,
        AntiAFK = _G.AntiAFK
    }
end

local function ApplySettingsTable(data)
    if not data or type(data) ~= "table" then return end
    pcall(function()
        if data.AutoKill ~= nil then _G.AutoKill = data.AutoKill if Elements.AutoKillToggle then Elements.AutoKillToggle:Set(data.AutoKill) end end
        if data.AutoStreakRestore ~= nil then _G.AutoStreakRestore = data.AutoStreakRestore if Elements.StreakRestoreToggle then Elements.StreakRestoreToggle:Set(data.AutoStreakRestore) end end
        if data.RematchLimit ~= nil then _G.RematchLimit = tonumber(data.RematchLimit) or 7 if Elements.RematchDropdown then Elements.RematchDropdown:Set({data.RematchLimit}) end end
        if data.AutoRematch ~= nil then _G.AutoRematch = data.AutoRematch if Elements.AutoRematchToggle then Elements.AutoRematchToggle:Set(data.AutoRematch) end end
        if data.AutoQueue ~= nil then _G.AutoQueue = data.AutoQueue if Elements.AutoQueueToggle then Elements.AutoQueueToggle:Set(data.AutoQueue) end end
        
        if data.SelectedModes then
            for mode, val in pairs(data.SelectedModes) do
                _G.SelectedModes[mode] = val
                if Elements["ModeToggle_"..mode] then Elements["ModeToggle_"..mode]:Set(val) end
            end
        end
        
        if data.AutoDuel ~= nil then _G.AutoDuelChallenge = data.AutoDuel if Elements.AutoDuelToggle then Elements.AutoDuelToggle:Set(data.AutoDuel) end end
        if data.AutoAccept ~= nil then _G.AutoAcceptDuel = data.AutoAccept if Elements.AutoAcceptToggle then Elements.AutoAcceptToggle:Set(data.AutoAccept) end end
        if data.AntiAFK ~= nil then _G.AntiAFK = data.AntiAFK if Elements.AntiAFKToggle then Elements.AntiAFKToggle:Set(data.AntiAFK) end end
    end)
end

local function TriggerPermanentAutoSave()
    local currentData = GetCurrentSettingsTable()
    writefile(PERMANENT_AUTOSAVE_FILE, HttpService:JSONEncode(currentData))
end

local function SaveConfigData(configName)
    if not configName or configName == "" or configName == "None" then return end
    local currentData = GetCurrentSettingsTable()
    writefile(CONFIG_FOLDER .. configName .. ".json", HttpService:JSONEncode(currentData))
end

local function LoadConfigData(configName)
    local path = CONFIG_FOLDER .. configName .. ".json"
    if not isfile(path) then return end
    local success, data = pcall(function() return HttpService:JSONDecode(readfile(path)) end)
    if success then ApplySettingsTable(data) end
end

local function DeleteConfigData(configName)
    if not configName or configName == "" or configName == "None" then return end
    local path = CONFIG_FOLDER .. configName .. ".json"
    if isfile(path) then delfile(path) end
end

-- [[ GAMEPLAY CORE FUNCTIONS ]]
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

local function IsAnyEnemyAlive()
    for _, p in ipairs(Players:GetPlayers()) do
        if p ~= LocalPlayer and p.Character and p.Character:FindFirstChild("Humanoid") and p.Character.Humanoid.Health > 0 and not p.Character:FindFirstChildOfClass("ForceField") then
            return true
        end
    end
    return false
end

local function KillAll()
    local Remotes = ReplicatedStorage:FindFirstChild("Remotes")
    if not Remotes then return end
    local ThrowRemote, HitRemote = Remotes:FindFirstChild("ThrowReplicate"), Remotes:FindFirstChild("ReportHit")
    if not ThrowRemote or not HitRemote then return end

    local myChar = LocalPlayer.Character
    local myPos = myChar and myChar:FindFirstChild("HumanoidRootPart") and myChar.HumanoidRootPart.Position
    if not myPos then return end

    for _, p in ipairs(Players:GetPlayers()) do
        if p ~= LocalPlayer and p.Character and p.Character:FindFirstChild("Humanoid") and p.Character.Humanoid.Health > 0 and not p.Character:FindFirstChildOfClass("ForceField") then
            local targetHead = p.Character:FindFirstChild("Head")
            if targetHead then
                local targetPos = targetHead.Position
                local currentId = math.random(1, 99999)
                pcall(function()
                    ThrowRemote:FireServer({["toolName"]="Knife", ["id"]=currentId, ["ownerUserId"]=LocalPlayer.UserId, ["origin"]=myPos, ["isExplosive"]=false, ["power"]=1, ["target"]=targetPos, ["effects"]={Shotgun=0, Portal=0, Smoke=0, Explosive=0, Flammable=0}})
                    HitRemote:FireServer({["hitPos"]=targetPos, ["ownerUserId"]=LocalPlayer.UserId, ["origin"]=myPos, ["vel"]=Vector3.new(100, 100, 100), ["headshot"]=true, ["targetUserId"]=p.UserId, ["targetModel"]=p.Character, ["to"]=targetPos, ["throwId"]=currentId, ["kind"]="throw", ["at"]=tick(), ["hitPart"]=targetHead})
                end)
            end
        end
    end
end

local function TryStreakRestore()
    if not _G.AutoStreakRestore then return end
    pcall(function()
        local StreakEvent = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("StreakRestore")
        if StreakEvent and StreakEvent:IsA("RemoteFunction") then
            StreakEvent:InvokeServer("Restore")
            Rayfield:Notify({Title = "Streak System", Content = "Streak Restore Automatically Triggered!", Duration = 3})
        end
    end)
end

local function OnRoundCleanup()
    TryStreakRestore()
    if not _G.AutoQueue and not _G.AutoDuelChallenge then return end
    _G.InCleanupWait = true
    task.delay(38, function() _G.InCleanupWait = false end)
end

local function TriggerSmartKill()
    WinLimitReached = false
    if not _G.AutoKill then return end
    
    if killThread then 
        task.cancel(killThread) 
        killThread = nil
    end
    
    OnRoundCleanup()
    
    killThread = task.spawn(function()
        task.wait(5.1)
        if IsAnyEnemyAlive() then
            KillAll()
            task.wait(2.5)
            if IsAnyEnemyAlive() then KillAll() end
        end
        killThread = nil
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

-- 1. MAIN TAB
MainTab:CreateSection("Main General Features")
Elements.StreakRestoreToggle = MainTab:CreateToggle({
    Name = "Auto Streak Restore (On Match End)",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoStreakRestore = Value
        TriggerPermanentAutoSave()
    end,
})

MainTab:CreateButton({
    Name = "Manual Streak Restore Now",
    Callback = function()
        pcall(function()
            local StreakEvent = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("StreakRestore")
            if StreakEvent then 
                StreakEvent:InvokeServer("Restore") 
                Rayfield:Notify({Title = "Streak System", Content = "Manual Restore Executed!", Duration = 3})
            end
        end)
    end,
})

MainTab:CreateSection("Security Settings")
Elements.AntiAFKToggle = MainTab:CreateToggle({
    Name = "Anti-AFK",
    CurrentValue = false,
    Callback = function(Value)
        _G.AntiAFK = Value
        TriggerPermanentAutoSave()
    end,
})

-- 2. COMBAT TAB
CombatTab:CreateSection("Kill Functions")
CombatTab:CreateButton({
    Name = "Kill All (Instant)",
    Callback = function() KillAll() end,
})

Elements.AutoKillToggle = CombatTab:CreateToggle({
    Name = "Auto Kill",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoKill = Value
        TriggerPermanentAutoSave()
    end,
})

-- 3. MATCHMAKING TAB
MatchmakingTab:CreateSection("Auto Rematch Settings")
Elements.RematchDropdown = MatchmakingTab:CreateDropdown({
    Name = "Win Limit (Server Hop)",
    Options = {"1", "2", "3", "4", "5", "6", "7", "8"},
    CurrentOption = {"7"},
    MultipleOptions = false,
    Callback = function(Option)
        _G.RematchLimit = tonumber(Option[1]) or 7
        TriggerPermanentAutoSave()
    end
})

Elements.AutoRematchToggle = MatchmakingTab:CreateToggle({
    Name = "Auto Rematch",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoRematch = Value
        TriggerPermanentAutoSave()
    end,
})

MatchmakingTab:CreateSection("Auto Queue")
Elements.AutoQueueToggle = MatchmakingTab:CreateToggle({
    Name = "Auto Queue",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoQueue = Value
        TriggerPermanentAutoSave()
        if _G.AutoQueue then 
            task.spawn(function() 
                while _G.AutoQueue do 
                    if not _G.InCleanupWait and not _G.IsMatched and not WinLimitReached then sendQueueSignal() end 
                    task.wait(10) 
                end 
            end) 
        end
    end,
})

MatchmakingTab:CreateSection("Select Modes")
for _, mode in ipairs({"1v1", "2v2", "3v3", "4v4"}) do
    Elements["ModeToggle_"..mode] = MatchmakingTab:CreateToggle({
        Name = "Mode: " .. mode,
        CurrentValue = false,
        Callback = function(Value)
            _G.SelectedModes[mode] = Value
            TriggerPermanentAutoSave()
        end,
    })
end

-- 4. MATCHMAKING(DUEL) TAB
MatchmakingDuelTab:CreateSection("Automatic Duel System (Queue Relay)")

local currentSelectedID = savedIDs[1] or ""

local QueueListLabel = MatchmakingDuelTab:CreateParagraph({
    Title = "Current Relay Queue",
    Content = "No IDs registered in queue."
})

local function UpdateQueueUI()
    if #_G.QueueList == 0 then
        QueueListLabel:Set({Title = "Current Relay Queue", Content = "Queue is empty. Add IDs below!"})
        _G.TargetDuelUserId = 0
        return
    end
    
    local text = ""
    for index, val in ipairs(_G.QueueList) do
        local prefix = (index == 1) and "🔥 [CURRENT TARGET] -> " or tostring(index) .. ". "
        text = text .. prefix .. tostring(val) .. "\n"
    end
    QueueListLabel:Set({Title = "Current Relay Queue", Content = text})
    
    _G.TargetDuelUserId = _G.QueueList[1] or 0
end

MatchmakingDuelTab:CreateInput({
    Name = "Add User ID to Queue",
    PlaceholderText = "Enter Target UserID...",
    RemoveTextAfterFocusLost = true,
    Callback = function(Text)
        local idNum = tonumber(Text)
        if idNum then
            saveID(Text)
            currentSelectedID = tostring(idNum)
            if Elements.IDDropdown then
                Elements.IDDropdown:Refresh(savedIDs, true)
                Elements.IDDropdown:Set({tostring(idNum)})
            end
            UpdateQueueUI()
        end
    end,
})

if #savedIDs == 0 then table.insert(savedIDs, "") end

Elements.IDDropdown = MatchmakingDuelTab:CreateDropdown({
    Name = "Select Saved ID",
    Options = savedIDs,
    CurrentOption = {currentSelectedID},
    MultipleOptions = false,
    Callback = function(Option)
        currentSelectedID = Option[1] or ""
        local selectedNum = tonumber(currentSelectedID)
        if selectedNum then
            for i, val in ipairs(_G.QueueList) do
                if val == selectedNum then
                    table.remove(_G.QueueList, i)
                    table.insert(_G.QueueList, 1, selectedNum)
                    break
                end
            end
            UpdateQueueUI()
        end
    end,
})

MatchmakingDuelTab:CreateButton({
    Name = "Delete Selected ID",
    Callback = function()
        if currentSelectedID and currentSelectedID ~= "" then
            deleteID(currentSelectedID)
            local nextID = savedIDs[1] or ""
            currentSelectedID = nextID
            if #savedIDs == 0 then table.insert(savedIDs, "") end
            if Elements.IDDropdown then
                Elements.IDDropdown:Refresh(savedIDs, true)
                Elements.IDDropdown:Set({nextID})
            end
            UpdateQueueUI()
        end
    end,
})

local function RotateQueue()
    if #_G.QueueList <= 1 then return end
    
    local finishedID = table.remove(_G.QueueList, 1) 
    table.insert(_G.QueueList, finishedID)          
    
    local newSaved = {}
    for _, id in ipairs(_G.QueueList) do
        table.insert(newSaved, tostring(id))
    end
    savedIDs = newSaved
    writefile(ID_CONFIG_FILE, HttpService:JSONEncode(savedIDs))
    
    if Elements.IDDropdown then
        Elements.IDDropdown:Refresh(savedIDs, true)
        Elements.IDDropdown:Set({tostring(_G.QueueList[1])})
    end
    
    UpdateQueueUI()
    
    Rayfield:Notify({
        Title = "Queue Rotation",
        Content = "Target changed! Next target: " .. tostring(_G.QueueList[1]),
        Duration = 5
    })
end

task.spawn(function()
    while true do
        task.wait(1)
        if not _G.IsMatched and _G.PendingRotation then
            _G.PendingRotation = false
            RotateQueue()
        end
    end
end)

Elements.AutoDuelToggle = MatchmakingDuelTab:CreateToggle({
    Name = "Auto Duel",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoDuelChallenge = Value
        TriggerPermanentAutoSave()
        if _G.AutoDuelChallenge then 
            task.spawn(function() 
                while _G.AutoDuelChallenge do 
                    local R = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("PlayerRequestSend") 
                    if _G.TargetDuelUserId and _G.TargetDuelUserId > 0 and R and not _G.IsMatched and not _G.InCleanupWait then 
                        pcall(function() R:InvokeServer({Type="Duel", TargetUserId=_G.TargetDuelUserId}) end) 
                        task.wait(10) 
                    else 
                        task.wait(2) 
                    end 
                end 
            end) 
        end
    end,
})

Elements.AutoAcceptToggle = MatchmakingDuelTab:CreateToggle({
    Name = "Auto Accept",
    CurrentValue = false,
    Callback = function(Value)
        _G.AutoAcceptDuel = Value
        TriggerPermanentAutoSave()
    end,
})

-- 5. SETTINGS TAB
SettingsTab:CreateSection("Configuration Management")
local currentConfigName = ""
local selectedConfig = "None"

SettingsTab:CreateInput({
    Name = "Config name",
    PlaceholderText = "Write config name here...",
    RemoveTextAfterFocusLost = false,
    Callback = function(Text) currentConfigName = Text end,
})

SettingsTab:CreateButton({
    Name = "Create config",
    Callback = function()
        if currentConfigName and currentConfigName ~= "" and currentConfigName ~= "None" then
            SaveConfigData(currentConfigName)
            Rayfield:Notify({Title = "Configuration", Content = "Config '"..currentConfigName.."' created!", Duration = 3})
            
            local newList = GetConfigList()
            if Elements.ConfigDropdown then
                Elements.ConfigDropdown:Refresh(newList, true)
                Elements.ConfigDropdown:Set({currentConfigName})
            end
        else
            Rayfield:Notify({Title = "Error", Content = "Please enter a valid config name.", Duration = 3})
        end
    end,
})

SettingsTab:CreateSection("Config list")
Elements.ConfigDropdown = SettingsTab:CreateDropdown({
    Name = "Select Config File",
    Options = GetConfigList(),
    CurrentOption = {"None"},
    MultipleOptions = false,
    Callback = function(Option) selectedConfig = Option[1] or "None" end,
})

SettingsTab:CreateButton({
    Name = "Load config",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            LoadConfigData(selectedConfig)
            Rayfield:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' loaded!", Duration = 3})
        end
    end,
})

SettingsTab:CreateButton({
    Name = "Overwrite config",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            SaveConfigData(selectedConfig)
            Rayfield:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' overwritten/saved!", Duration = 3})
        end
    end,
})

SettingsTab:CreateButton({
    Name = "Delete config",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            DeleteConfigData(selectedConfig)
            Rayfield:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' deleted.", Duration = 3})
            
            local newList = GetConfigList()
            if Elements.ConfigDropdown then
                Elements.ConfigDropdown:Refresh(newList, true)
                Elements.ConfigDropdown:Set({"None"})
            end
        end
    end,
})

SettingsTab:CreateButton({
    Name = "Refresh list",
    Callback = function()
        local newList = GetConfigList()
        if Elements.ConfigDropdown then
            Elements.ConfigDropdown:Refresh(newList, true)
            Rayfield:Notify({Title = "Configuration", Content = "Config list refreshed.", Duration = 2})
        end
    end,
})

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
        
        -- [[ ⭐ 제공해주신 원본 '작동 보장' 순정 구조 그대로 결합 고정 ]]
        if RequestNotify and RespondRemote then
            RequestNotify.OnClientEvent:Connect(function(data)
                if not _G.AutoAcceptDuel then return end
                
                local requestId = nil
                if type(data) == "string" then 
                    requestId = data
                elseif type(data) == "table" then
                    requestId = data.Id or data.id or data.UUID or data.RequestId
                    if not requestId then
                        for _, v in pairs(data) do
                            if type(v) == "string" and #v == 36 and string.find(v, "-") then 
                                requestId = v 
                                break 
                            end
                        end
                    end
                end
                
                if requestId then
                    task.wait(0.25)
                    pcall(function() 
                        RespondRemote:FireServer(requestId, true) 
                        Rayfield:Notify({Title = "Auto Accept", Content = "Duel Accepted!", Duration = 2})
                    end)
                end
            end)
        end

        -- [[ KILLFEED_PUSH 기반 자동 릴레이 감지 장치 ]]
        local KillFeedPush = Remotes:WaitForChild("KillFeed_Push", 10)
        if KillFeedPush then
            KillFeedPush.OnClientEvent:Connect(function(data)
                if not data or type(data) ~= "table" then return end
                if #_G.QueueList == 0 then return end
                
                local victim = data.victim
                local killer = data.killer
                local types = data.types
                
                if not victim or not killer or not types then return end
                
                local isMyKill = false
                if type(killer) == "userdata" and killer == LocalPlayer then
                    isMyKill = true
                elseif type(killer) == "table" and (killer.UserId == LocalPlayer.UserId or killer.userId == LocalPlayer.UserId) then
                    isMyKill = true
                end
                if not isMyKill then return end
                
                local currentTargetID = _G.QueueList[1]
                local victimID = tonumber(victim.userId or victim.UserId)
                if victimID ~= currentTargetID then return end
                
                local totalXP = 0
                for _, reward in ipairs(types) do
                    if reward.xp then
                        totalXP = totalXP + tonumber(reward.xp)
                    end
                end
                
                if totalXP == 0 then
                    Rayfield:Notify({
                        Title = "Anti-Abuse Locked Detected",
                        Content = "XP reward is 0! Rotating targets...",
                        Duration = 5
                    })
                    
                    if _G.IsMatched then
                        _G.PendingRotation = true
                    else
                        RotateQueue()
                    end
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

-- 백엔드 실시간 세이브 로드 연동
task.spawn(function() 
    task.wait(0.8)
    if isfile(PERMANENT_AUTOSAVE_FILE) then
        local success, data = pcall(function() return HttpService:JSONDecode(readfile(PERMANENT_AUTOSAVE_FILE)) end)
        if success and type(data) == "table" then
            ApplySettingsTable(data)
        end
    end
    
    UpdateQueueUI()
    if Elements.IDDropdown and savedIDs and #savedIDs > 0 and savedIDs[1] ~= "" then
        Elements.IDDropdown:Refresh(savedIDs, true)
        currentSelectedID = savedIDs[1] or ""
        _G.TargetDuelUserId = tonumber(currentSelectedID) or 0
        Elements.IDDropdown:Set({currentSelectedID})
    end
    Rayfield:Notify({ Title = "Koji HUD", Content = "Systems Initialized Successfully" })
end)
