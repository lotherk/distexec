#!/usr/bin/env ruby
#
refactor = [
	['dex_', 'libdistexec_'],
	['DEX_EXPORT', 'EXPORT'],
	['DEX', 'LIBDISTEXEC'],
]
ARGV.each do |argv|
	Dir[File.join(argv, '**', '*')].each do |file|
		next if File.directory?(file)
		next if file =~ /cmdline/
		next if file =~ /config/

		fp = File.read(file)
		i = 0
		buffer = []

		fp.each_line do |line|
			new_line = line.clone
			modified = false
			refactor.each do |r|
				if new_line.match(Regexp.new(r[0]))
					new_line = new_line.gsub(r[0], r[1])
					modified = true
				end
			end
			if modified
				puts
				puts File.basename(file) + ":" + i.to_s + " >> " + line
				puts File.basename(file) + ":" + i.to_s + " << " + new_line
			end
			buffer[i] = new_line
			i += 1
		end
		File.unlink(file)
		File.open(file, 'w') do |fp|
			fp.write(buffer.join(""))
		end
	end
end
