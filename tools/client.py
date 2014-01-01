# Echo server program
import socket
import struct

HOST = '127.0.0.1'    # The remote host
PORT = 7777              # The same port as used by the server

isquit=False

while True:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5.0)
    s.connect((HOST, PORT))
    print('connect success')
    while True:
        try:
            cmd=raw_input('#')
            param=cmd.split(' ')
            mycmd=''
            for i in range(len(param)):
                mycmd=mycmd+param[i]
                mycmd=mycmd+' '
            mycmd=mycmd+"\r\n"
            s.sendall(mycmd)
            data = s.recv(1024)
            print(data)
        except:
            print('lose connection')
            cmd=raw_input("?reconnect[yes/no]")
            if cmd == "yes":
                break
            else:
                isquit=True
                break
    if isquit:
        break
    s.close()


