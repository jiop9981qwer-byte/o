local Rayfield = loadstring(game:HttpGet('https://sirius.menu/rayfield'))()
local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local VirtualUser = game:GetService("VirtualUser")
local LocalPlayer = Players.LocalPlayer

-- [[ GLOBAL STATES ]]
_G.AutoAcceptDuel = false

-- [[ UI SETUP ]]
local Window = Rayfield:CreateWindow({
    Name = "Koji HUD - Duel Auto",
    LoadingTitle = "Initializing...",
    LoadingSubtitle = "by Koji",
    ConfigurationSaving = { Enabled = true, FolderName = "KojiHUD", FileName = "DuelConfig" }
})

local Tab = Window:CreateTab("Combat", 4483362458)
Tab:CreateToggle({
    Name = "Auto Accept Duel",
    CurrentValue = false,
    Callback = function(Value) _G.AutoAcceptDuel = Value end,
})

-- [[ CORE AUTO ACCEPT LOGIC ]]
task.spawn(function()
    local Remotes = ReplicatedStorage:WaitForChild("Remotes", 10)
    local NotifyEvent = Remotes and Remotes:WaitForChild("PlayerRequestNotify", 10)
    local RespondEvent = Remotes and Remotes:WaitForChild("PlayerRequestRespond", 10)

    if NotifyEvent and RespondEvent then
        NotifyEvent.OnClientEvent:Connect(function(data)
            if not _G.AutoAcceptDuel then return end
            
            local requestId = nil
            
            -- 서버에서 넘어온 데이터에서 UUID 형식(36자)의 문자열을 찾아 추출합니다.
            if type(data) == "string" then
                requestId = data
            elseif type(data) == "table" then
                -- 일반적인 키 탐색
                requestId = data.Id or data.id or data.UUID or data.RequestId
                
                -- 키가 안 맞을 경우 테이블 내부에서 36자 길이의 UUID를 강제 검색
                if not requestId then
                    for _, v in pairs(data) do
                        if type(v) == "string" and #v == 36 and string.find(v, "-") then
                            requestId = v
                            break
                        end
                    end
                end
            end

            -- ID가 확인되면 즉시 수락 패킷 전송
            if requestId then
                task.wait(0.25) -- 네트워크 지연 방지를 위한 짧은 딜레이
                pcall(function()
                    RespondEvent:FireServer(requestId, true)
                end)
            end
        end)
    end
end)

-- [[ ANTI-AFK ]]
LocalPlayer.Idled:Connect(function()
    VirtualUser:CaptureController()
    VirtualUser:ClickButton2(Vector2.new())
end)

Rayfield:Notify({ Title = "Success", Content = "Auto Accept System Active" })
