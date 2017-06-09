#!/usr/bin/env ruby

obj = open(ARGV[0], "ab")

if obj.size <= 510
    fill = 510 - obj.size
    fill.times { obj.write("\x00") }
    obj.write("\x55")
    obj.write("\xaa")
else
    puts "#{ARGV[0]}'s size is too large"
end
