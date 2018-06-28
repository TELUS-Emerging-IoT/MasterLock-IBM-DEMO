import serial
import time
from recognition_tutorial import initialization, recognize

nucleoSerial = serial.Serial("/dev/ttyACM0", 9600, timeout=1)

if __name__ == "__main__":
	#train data
	print("TELUS FaceUnlock is initializing...")
	initialization()
	nucleoSerial.write(b'-')

	#main loop
	print("Program is ready to begin.")
	print("Looking for facial unlock trigger...")
	nucleoSerial.flushInput()
	while True:
		if (nucleoSerial.inWaiting() > 0):
			command = nucleoSerial.readline()
			print(command)
			if (command == 'c'):
				print("Face detected, beginning recognition...")
				if (recognize() == "s1"): 
					nucleoSerial.write(b'o')
					print("Unlocked.")	
				else:
					nucleoSerial.write(b'x')
					print("Unauthorized or unsafe user, access denied.")
				nucleoSerial.flushInput()
