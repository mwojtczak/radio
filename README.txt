Computer Networks Classes Assignment
The goal was to implement radio player.
With tcp connection we get audio data (optionaly with metadata) from given source. User can give commands via UPD.
Avaliable commands:
PAUSE - pauses passing audio data to file/output
PLAY - starts passing audio data
TITLE - returns title of current song (if metadata were on)
QUIT - quits program player

HOW TO RUN
First run Makefile.
Then run player:
	./player host path r-port file m-port md
where
	host - server name providing audio stream
	path - name of resource
	r-port - port of server providing audio stream
	file - filename to save audio data or '-'
	m-port - udp port to be the source of commands
	md - whether to check metadata, 'yes' or 'no'

Example of usage:
	./player ant-waw-01.cdn.eurozet.pl / 8602 test.mp3 3500 yes

Then connect to port 3500 via nc:
	nc -u localhost 3500
to be able to send commands to player.




