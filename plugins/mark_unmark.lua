
function onInput()
    local key = getKey()
    local cursor = getCursorFilePosition()
    if key == "i" then
        mark(cursor, 1)
    elseif key == "o" then
        unmark(cursor)
    end
end

autoload()