# original script by Zach Lipp

import os

if __name__ == '__main__':

	info = dict()
    
	modes = ( 'rand', 'custom', 'fifo' )
	functions = ( 'scan', 'sort', 'focus' )
    
	for func in functions:

		for mode in modes:

			filestr = "./test_results/" + mode + "_" + func + ".csv"
			outfile = open( filestr, "w" )
			outfile.write( "Num Frames, Page Faults, Disk Reads, Disk Writes\n" )
            
			for nframes in range(2,101):

				command = "./virtmem 100 "+str(nframes)+" "+mode+" "+func
				print command

				output = os.popen(command)
				lines = output.readlines()		#lines gets array of lines in output
				line = lines[1]
				line = line.strip()
				print line

				arr = line.split( ',' )
				page_faults = arr[0]
				disk_rd = arr[1]
				disk_wr = arr[2]
				print arr

				outline = str( nframes ) + "," + str( page_faults ) + "," + str( disk_rd ) + "," + str( disk_wr ) + "\n"; 
				outfile.write( outline )

			outfile.close()

