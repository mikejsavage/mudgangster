local i = 0
for c in io.stdin:lines( 1 ) do
	if i > 0 and i % 16 == 0 then
		print()
	end
	i = i + 1
	io.stdout:write( string.byte( c ) .. "," )
end
