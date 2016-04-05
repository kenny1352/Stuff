from datetime import datetime
from socket import *
serverName = '127.0.0.1'
serverPort = 12000

i = 0
while (i < 11):
    i += 1
    clientSocket = socket(AF_INET, SOCK_DGRAM)
    time = datetime.now()
    message = 'Ping %i %s' % (i, time.strftime("%H:%M:%S"))
    clientSocket.sendto(message,(serverName, serverPort))
    clientSocket.settimeout(1)
    try:
        returnMessage, serverAddress = clientSocket.recvfrom(2048)
        time2 = datetime.now() - time
        print returnMessage
        print 'Ping',i,' RTT =',time2
    except:
        print "Request timed out"
    clientSocket.close()
