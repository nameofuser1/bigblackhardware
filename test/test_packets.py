import socket
from time import sleep


if __name__ == "__main__":
    host_addr = ('192.168.1.43', 1000)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(host_addr)

    packet = bytearray([0x1B, 0, 0x24, 0, 4, 0x01, 0x02, 0x03, 0x04])
    sock.send(packet)

    sleep(5)
    sock.close()
