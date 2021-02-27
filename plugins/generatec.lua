
function generateCFile() 
    local filename = getTextInput("Enter Filename") .. ".c"
    io.output(filename)
    io.write("unsigned char* data = {")
    for i=0, getFileSize()-1 do
        if (i % 16 == 0) then
            io.write("\n\t")
        end
        
        io.write(string.format("0x%02x", file[i]))
        if i ~= getFileSize()-1 then
            io.write(", ")
        end
    end
    io.write("\n}")
    io.close()
    log(filename .. " was successfully generated!")
    showConsole()
end

registerMenuCallback("Generate C-File", "generateCFile")
autoload()