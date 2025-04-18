
first = true
local lastAlarmsTime = 0

function notify0()
    if first then
        return true, false
    end
    local day = tonumber(os.date("%d"))
    local hour = tonumber(os.date("%H"))
    local min = tonumber(os.date("%M"))
    if day == 1 and hour == 8 and min < 10 then
        return true, false
    end
    return false, false
end

function notify1()
    return true, true
end

function alarmsTime()
    if os.time() - lastAlarmsTime > 3600 then
        return true
    end
    return false
end

function alarms0()
    if alarmsTime() then
        lastAlarmsTime = os.time()
        return true, false
    end
    return false, false
end

function alarms1()
    if alarmsTime() then
        lastAlarmsTime = os.time()
        return true, false
    end
    return false, false
end

function alarms2()
    if alarmsTime() then
        lastAlarmsTime = os.time()
        return true, false
    end
    return false, false
end

function alarms3()
    if alarmsTime() then
        lastAlarmsTime = os.time()
        return true, false
    end
    return false, false
end
