local Fluent = loadstring(game:HttpGet("https://github.com/dawid-scripts/Fluent/releases/latest/download/main.lua"))()
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
_G.AutoStreakRestore = false  -- 연승 복구 토글 변수
_G.RematchLimit = 7
_G.SelectedModes = {["1v1"] = false, ["2v2"] = false, ["3v3"] = false, ["4v4"] = false}
_G.IsMatched = false
_G.InCleanupWait = false

local killThread = nil

-- [[ FILE STORAGE PATHS ]]
local ID_CONFIG_FILE = "KojiHUD_SavedIDs.json"
local CONFIG_FOLDER = "KojiHUD_Configs/"
local PERMANENT_AUTOSAVE_FILE = "KojiHUD_AutoSave.json" -- 완전 자동 저장용 파일

if not isfolder(CONFIG_FOLDER) then makefolder(CONFIG_FOLDER) end

-- [[ ID STORAGE LOGIC ]]
local savedIDs = {}
local function loadSavedIDs()
    if isfile(ID_CONFIG_FILE) then
        local success, result = pcall(function() return HttpService:JSONDecode(readfile(ID_CONFIG_FILE)) end)
        if success and type(result) == "table" then savedIDs = result else savedIDs = {} end
    else savedIDs = {} end
end

local function saveID(idText)
    local idNum = tonumber(idText)
    if not idNum then return end
    for _, val in ipairs(savedIDs) do if val == tostring(idNum) then return end end
    table.insert(savedIDs, tostring(idNum))
    writefile(ID_CONFIG_FILE, HttpService:JSONEncode(savedIDs))
end

local function deleteID(idText)
    if not idText or idText == "" then return end
    for i, val in ipairs(savedIDs) do
        if val == idText then table.remove(savedIDs, i) break end
    end
    writefile(ID_CONFIG_FILE, HttpService:JSONEncode(savedIDs))
end

loadSavedIDs()

-- [[ UI SETUP ]]
local Window = Fluent:CreateWindow({
    Title = "Koji HUD",
    SubTitle = "by Koji",
    TabWidth = 160,
    Size = UDim2.fromOffset(580, 580),
    Acrylic = true,
    Theme = "Dark",
    MinimizeKey = Enum.KeyCode.LeftControl
})

-- 탭 구성 (종합 기능인 Main 탭을 상단에 추가)
local MainTab = Window:AddTab({ Title = "Main", Icon = "home" })
local CombatTab = Window:AddTab({ Title = "Combat", Icon = "sword" })
local MatchmakingTab = Window:AddTab({ Title = "Matchmaking", Icon = "user" })
local SettingsTab = Window:AddTab({ Title = "Settings", Icon = "settings" })

local Options = Fluent.Options

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

-- UI의 모든 현재 세팅 값을 가져오는 함수
local function GetCurrentSettingsTable()
    return {
        AutoKill = Options.AutoKillToggle and Options.AutoKillToggle.Value or false,
        AutoStreakRestore = Options.StreakRestoreToggle and Options.StreakRestoreToggle.Value or false,
        RematchLimit = Options.RematchDropdown and Options.RematchDropdown.Value or "7",
        AutoRematch = Options.AutoRematchToggle and Options.AutoRematchToggle.Value or false,
        AutoQueue = Options.AutoQueueToggle and Options.AutoQueueToggle.Value or false,
        SelectedModes = {
            ["1v1"] = Options.ModeToggle_1v1 and Options.ModeToggle_1v1.Value or false,
            ["2v2"] = Options.ModeToggle_2v2 and Options.ModeToggle_2v2.Value or false,
            ["3v3"] = Options.ModeToggle_3v3 and Options.ModeToggle_3v3.Value or false,
            ["4v4"] = Options.ModeToggle_4v4 and Options.ModeToggle_4v4.Value or false,
        },
        AutoDuel = Options.AutoDuelToggle and Options.AutoDuelToggle.Value or false,
        AutoAccept = Options.AutoAcceptToggle and Options.AutoAcceptToggle.Value or false,
        AntiAFK = Options.AntiAFKToggle and Options.AntiAFKToggle.Value or false
    }
end

-- 테이블 데이터를 바탕으로 UI 컴포넌트 값을 복구하는 함수
local function ApplySettingsTable(data)
    if not data or type(data) ~= "table" then return end
    pcall(function()
        if Options.AutoKillToggle and data.AutoKill ~= nil then Options.AutoKillToggle:SetValue(data.AutoKill) end
        if Options.StreakRestoreToggle and data.AutoStreakRestore ~= nil then Options.StreakRestoreToggle:SetValue(data.AutoStreakRestore) end
        if Options.RematchDropdown and data.RematchLimit ~= nil then Options.RematchDropdown:SetValue(data.RematchLimit) end
        if Options.AutoRematchToggle and data.AutoRematch ~= nil then Options.AutoRematchToggle:SetValue(data.AutoRematch) end
        if Options.AutoQueueToggle and data.AutoQueue ~= nil then Options.AutoQueueToggle:SetValue(data.AutoQueue) end
        
        if data.SelectedModes then
            if Options.ModeToggle_1v1 and data.SelectedModes["1v1"] ~= nil then Options.ModeToggle_1v1:SetValue(data.SelectedModes["1v1"]) end
            if Options.ModeToggle_2v2 and data.SelectedModes["2v2"] ~= nil then Options.ModeToggle_2v2:SetValue(data.SelectedModes["2v2"]) end
            if Options.ModeToggle_3v3 and data.SelectedModes["3v3"] ~= nil then Options.ModeToggle_3v3:SetValue(data.SelectedModes["3v3"]) end
            if Options.ModeToggle_4v4 and data.SelectedModes["4v4"] ~= nil then Options.ModeToggle_4v4:SetValue(data.SelectedModes["4v4"]) end
        end
        
        if Options.AutoDuelToggle and data.AutoDuel ~= nil then Options.AutoDuelToggle:SetValue(data.AutoDuel) end
        if Options.AutoAcceptToggle and data.AutoAccept ~= nil then Options.AutoAcceptToggle:SetValue(data.AutoAccept) end
        if Options.AntiAFKToggle and data.AntiAFK ~= nil then Options.AntiAFKToggle:SetValue(data.AntiAFK) end
    end)
end

-- [완전 실시간 자동저장 기능]
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

-- [[ CORE FUNCTIONS ]]
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

-- 연승 복구 실행 함수
local function TryStreakRestore()
    if not _G.AutoStreakRestore then return end
    pcall(function()
        local StreakEvent = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("StreakRestore")
        if StreakEvent and StreakEvent:IsA("RemoteFunction") then
            StreakEvent:InvokeServer("Restore")
            Fluent:Notify({Title = "Streak System", Content = "Streak Restore Automatically Triggered!", Duration = 3})
        end
    end)
end

local function OnRoundCleanup()
    -- 매치 종료 시점 처리
    TryStreakRestore()
    
    if not _G.AutoQueue and not _G.AutoDuelChallenge then return end
    _G.InCleanupWait = true
    task.delay(38, function() _G.InCleanupWait = false end)
end

local function TriggerSmartKill()
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

-- 1. MAIN TAB (종합 기능 칸)
MainTab:AddSection("Main General Features")
local StreakRestoreToggle = MainTab:AddToggle("StreakRestoreToggle", { Title = "Auto Streak Restore (On Match End)", Default = false })
StreakRestoreToggle:OnChanged(function()
    _G.AutoStreakRestore = StreakRestoreToggle.Value
    TriggerPermanentAutoSave() -- 값 변경 시 실시간 세이브
end)

MainTab:AddButton({
    Title = "Manual Streak Restore Now",
    Callback = function()
        pcall(function()
            local StreakEvent = ReplicatedStorage:FindFirstChild("Remotes") and ReplicatedStorage.Remotes:FindFirstChild("StreakRestore")
            if StreakEvent then 
                StreakEvent:InvokeServer("Restore") 
                Fluent:Notify({Title = "Streak System", Content = "Manual Restore Executed!", Duration = 3})
            end
        end)
    end
})

-- 2. COMBAT TAB
CombatTab:AddSection("Kill Functions")
CombatTab:AddButton({ Title = "Kill All (Instant)", Callback = function() KillAll() end })

local AutoKillToggle = CombatTab:AddToggle("AutoKillToggle", { Title = "Auto Kill", Default = false })
AutoKillToggle:OnChanged(function() 
    _G.AutoKill = AutoKillToggle.Value 
    TriggerPermanentAutoSave() -- 값 변경 시 실시간 세이브
end)

-- 3. MATCHMAKING TAB
MatchmakingTab:AddSection("Auto Rematch Settings")
local RematchDropdown = MatchmakingTab:AddDropdown("RematchDropdown", {
    Title = "Win Limit (Server Hop)",
    Values = {"1", "2", "3", "4", "5", "6", "7", "8"},
    CurrentValue = "7"
})
RematchDropdown:OnChanged(function(Value) 
    _G.RematchLimit = tonumber(Value) 
    TriggerPermanentAutoSave()
end)

local AutoRematchToggle = MatchmakingTab:AddToggle("AutoRematchToggle", { Title = "Auto Rematch", Default = false })
AutoRematchToggle:OnChanged(function() 
    _G.AutoRematch = AutoRematchToggle.Value 
    TriggerPermanentAutoSave()
end)

MatchmakingTab:AddSection("Auto Queue")
local AutoQueueToggle = MatchmakingTab:AddToggle("AutoQueueToggle", { Title = "Auto Queue", Default = false })
AutoQueueToggle:OnChanged(function() 
    _G.AutoQueue = AutoQueueToggle.Value 
    TriggerPermanentAutoSave()
    if _G.AutoQueue then 
        task.spawn(function() 
            while _G.AutoQueue do 
                if not _G.InCleanupWait and not _G.IsMatched then sendQueueSignal() end 
                task.wait(10) 
            end 
        end) 
    end
end)

MatchmakingTab:AddSection("Select Modes")
for _, mode in ipairs({"1v1", "2v2", "3v3", "4v4"}) do
    local ModeToggle = MatchmakingTab:AddToggle("ModeToggle_"..mode, { Title = "Mode: " .. mode, Default = false })
    ModeToggle:OnChanged(function() 
        _G.SelectedModes[mode] = ModeToggle.Value 
        TriggerPermanentAutoSave()
    end)
end

MatchmakingTab:AddSection("Automatic Duel System")
local IDDropdown
local currentSelectedID = savedIDs[1] or ""

MatchmakingTab:AddInput("AddIDInput", {
    Title = "Add User ID",
    Placeholder = "Enter Target UserID...",
    Numeric = true,
    Finished = true,
    Callback = function(Text)
        local idNum = tonumber(Text)
        if idNum then
            _G.TargetDuelUserId = idNum
            saveID(Text)
            currentSelectedID = tostring(idNum)
            if IDDropdown then
                IDDropdown:SetValues(savedIDs)
                IDDropdown:SetValue(tostring(idNum))
            end
        end
    end
})

if #savedIDs == 0 then table.insert(savedIDs, "") end

IDDropdown = MatchmakingTab:AddDropdown("IDDropdown", {
    Title = "Select Saved ID",
    Values = savedIDs,
    CurrentValue = currentSelectedID
})
IDDropdown:OnChanged(function(Value)
    currentSelectedID = Value
    _G.TargetDuelUserId = tonumber(Value) or 0
end)

MatchmakingTab:AddButton({
    Title = "Delete Selected ID",
    Callback = function()
        if currentSelectedID and currentSelectedID ~= "" then
            deleteID(currentSelectedID)
            local nextID = savedIDs[1] or ""
            currentSelectedID = nextID
            _G.TargetDuelUserId = tonumber(nextID) or 0
            if #savedIDs == 0 then table.insert(savedIDs, "") end
            if IDDropdown then
                IDDropdown:SetValues(savedIDs)
                IDDropdown:SetValue(nextID)
            end
        end
    end
})

local AutoDuelToggle = MatchmakingTab:AddToggle("AutoDuelToggle", { Title = "Auto Duel", Default = false })
AutoDuelToggle:OnChanged(function()
    _G.AutoDuelChallenge = AutoDuelToggle.Value
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
end)

local AutoAcceptToggle = MatchmakingTab:AddToggle("AutoAcceptToggle", { Title = "Auto Accept", Default = false })
AutoAcceptToggle:OnChanged(function() 
    _G.AutoAcceptDuel = AutoAcceptToggle.Value 
    TriggerPermanentAutoSave()
end)

MatchmakingTab:AddSection("Misc")
local AntiAFKToggle = MatchmakingTab:AddToggle("AntiAFKToggle", { Title = "Anti-AFK", Default = false })
AntiAFKToggle:OnChanged(function() 
    _G.AntiAFK = AntiAFKToggle.Value 
    TriggerPermanentAutoSave()
end)


-- 4. SETTINGS TAB (정밀 세팅 섹션)
SettingsTab:AddSection("Configuration")

local currentConfigName = ""
local selectedConfig = "None"
local ConfigDropdown

SettingsTab:AddInput("ConfigNameInput", {
    Title = "Config Name",
    Placeholder = "Write config name here...",
    Finished = true,
    Callback = function(Text) currentConfigName = Text end
})

SettingsTab:AddButton({
    Title = "Create Config",
    Callback = function()
        if currentConfigName and currentConfigName ~= "" and currentConfigName ~= "None" then
            SaveConfigData(currentConfigName)
            Fluent:Notify({Title = "Configuration", Content = "Config '"..currentConfigName.."' created!", Duration = 3})
            
            -- 드롭다운 컴포넌트 내부 리스트 데이터 직접 교체 연동 구현
            local newList = GetConfigList()
            ConfigDropdown.Values = newList
            ConfigDropdown:BuildDropdownList()
            ConfigDropdown:SetValue(currentConfigName)
        else
            Fluent:Notify({Title = "Error", Content = "Please enter a valid config name.", Duration = 3})
        end
    end
})

SettingsTab:AddSection("Config List")

ConfigDropdown = SettingsTab:AddDropdown("ConfigDropdown", {
    Title = "Select Config File",
    Values = GetConfigList(),
    CurrentValue = "None"
})
ConfigDropdown:OnChanged(function(Value) selectedConfig = Value end)

SettingsTab:AddButton({
    Title = "Load Config",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            LoadConfigData(selectedConfig)
            Fluent:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' loaded!", Duration = 3})
        end
    end
})

SettingsTab:AddButton({
    Title = "Overwrite Config (Save)",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            SaveConfigData(selectedConfig)
            Fluent:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' saved!", Duration = 3})
        end
    end
})

SettingsTab:AddButton({
    Title = "Delete Config",
    Callback = function()
        if selectedConfig and selectedConfig ~= "None" then
            DeleteConfigData(selectedConfig)
            Fluent:Notify({Title = "Configuration", Content = "Config '"..selectedConfig.."' deleted.", Duration = 3})
            
            local newList = GetConfigList()
            ConfigDropdown.Values = newList
            ConfigDropdown:BuildDropdownList()
            ConfigDropdown:SetValue("None")
        end
    end
})

SettingsTab:AddButton({
    Title = "Refresh List",
    Callback = function()
        local newList = GetConfigList()
        ConfigDropdown.Values = newList
        ConfigDropdown:BuildDropdownList()
        Fluent:Notify({Title = "Configuration", Content = "Config list refreshed.", Duration = 2})
    end
})


-- [[ MAIN LOGIC & BACKEND CONNECTIONS ]]
task.spawn(function()
    local Remotes = ReplicatedStorage:WaitForChild("Remotes", 20)
    local MatchmakingRemotes = ReplicatedStorage:WaitForChild("MatchmakingShared", 20) and ReplicatedStorage.MatchmakingShared:WaitForChild("Remotes", 20)
    if Remotes then
        Remotes:WaitForChild("RoundCleanup", 10).OnClientEvent:Connect(TriggerSmartKill)
        Remotes:WaitForChild("ClientLoaded", 10).OnClientEvent:Connect(TriggerSmartKill)
        Remotes:WaitForChild("PlayerRequestNotify", 10).OnClientEvent:Connect(function(data) if _G.AutoAcceptDuel then local R = Remotes:WaitForChild("PlayerRequestRespond", 10) task.wait(0.25) pcall(function() R:FireServer(data.Id or data.id or data.UUID or data.RequestId, true) end) end end)
    end
    if MatchmakingRemotes then
        MatchmakingRemotes:WaitForChild("PartyStateChanged", 10).OnClientEvent:Connect(function(data) if data then _G.IsMatched = data.matched end end)
        MatchmakingRemotes:WaitForChild("RematchState", 10).OnClientEvent:Connect(function(data) if data and (data.winsA >= _G.RematchLimit or data.winsB >= _G.RematchLimit) then task.spawn(function() for i=1,3 do sendQueueSignal() task.wait(2) end ServerHop() end) end end)
    end
end)

LocalPlayer.Idled:Connect(function() if _G.AntiAFK then VirtualUser:CaptureController() VirtualUser:ClickButton2(Vector2.new()) end end)

-- 실행 시 자동 복구 로드 시스템
task.spawn(function() 
    task.wait(0.6)
    
    -- [Rayfield 스타일 완전 자동복구 기능 적용]
    if isfile(PERMANENT_AUTOSAVE_FILE) then
        local success, data = pcall(function() return HttpService:JSONDecode(readfile(PERMANENT_AUTOSAVE_FILE)) end)
        if success and type(data) == "table" then
            ApplySettingsTable(data)
            Fluent:Notify({Title = "Auto Load", Content = "Your previous options have been automatically restored!", Duration = 4})
        end
    end
    
    -- 타겟 ID 목록 복구
    if IDDropdown and savedIDs and #savedIDs > 0 and savedIDs[1] ~= "" then
        IDDropdown:SetValues(savedIDs)
        currentSelectedID = savedIDs[1] or ""
        _G.TargetDuelUserId = tonumber(currentSelectedID) or 0
        IDDropdown:SetValue(currentSelectedID)
    end
end)

Window:SelectTab(1)
