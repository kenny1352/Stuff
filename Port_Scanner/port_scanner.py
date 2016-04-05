import argparse
import sys, time
from socket import *

def scan_ports(host, start_port, end_port, protocol):
    #setup
    print "Scanning: " +host +" From port: " +str(start_port) +" To Port: " +str(end_port)
    #Create Socket
    try:
	#Create a TCP or UDP socket depending on protocol
    	if (protocol == 'TCP'):
	    serverSocket = socket(AF_INET, SOCK_STREAM)
    	else:
	    serverSocket = socket(AF_INET, DGRAM)
    except:
    #handle the error
	print("ERROR: Connection could not be made")
    #Set the IP
    remote_ip=host

    #Scan Ports
    end_port +=1
    #loop over the range of ports
    for(start_Port; start_port < end_port; start_port++): 
	# If protocol is TCP
	if (protocol == 'TCP'):
        	#try TCP port detect if open
		
	# else:
	else:
		#UPD think about your UDP ping programs  

                
#parsing stuff you dont have to change
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Remote Port Scanner')
    parser.add_argument('--host', action="store", dest="host", default='127.0.0.1')
    parser.add_argument('--start-port', action="store", dest="start_port", default=1, type=int)
    parser.add_argument('--end-port', action="store", dest="end_port", default=100, type=int)
    parser.add_argument('--protocol', action="store", dest="protocol", default="TCP")
#parse args
    given_args = parser.parse_args()
    host, start_port, end_port, protocol = given_args.host, given_args.start_port, given_args.end_port, given_args.protocol
    scan_ports(host, start_port, end_port, protocol)
