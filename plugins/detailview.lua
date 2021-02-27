
function onInput()
    if getKey() == " " then
        toggle = not toggle
    end
    
    if toggle then
        local ex, ey, ew, eh = getEditorBounds()
        local cx, cy = getCursorWindowPosition()
        local pos = getCursorFilePosition()
        local size = getFileSize()
        
        clearWindow(window)
        setWindowBounds(window, ex + cx, ey + cy + 1, 40, 6)
        
        local u8 = file[pos]
        local u16
        local u32
        local u64
        printWindow(window, 1, 1, string.format("unsigned char: %d", u8), 1)
        if pos + 1 < size then
            u16 = (u8 << 8) | file[pos + 1]
            printWindow(window, 1, 2, string.format("unsigned short: %d", u16), 1)
        end
        if pos + 3 < size then
            u32 = (u16 << 16) | (file[pos + 2] << 8) | file[pos + 3]
            printWindow(window, 1, 3, string.format("unsigned int: %d", u32), 1)
        end
        if pos + 7 < size then
            u64 = (u32 << 32) | (file[pos + 4] << 24) | (file[pos + 5] << 16) | (file[pos + 6] << 8) | file[pos + 7]
            printWindow(window, 1, 4, string.format("unsigned long: %d", u64), 1)
        end
        setVisible(window, true)
    else 
        setVisible(window, false)
    end
end

toggle = false
window = newWindow(0, 0, 0, 0)
setVisible(window, false)
autoload()