import sys
# import threading
import utils
from scu import SCU

RECEIVER_ADDRESS = "127.0.0.1"
PORT = 8888

def main():
    if sys.argv[1] == "sender":
        scu = SCU(mtu=1500)
        scu.bind_as_sender(receiver_address=(RECEIVER_ADDRESS, PORT))
        try:
            # serial
            for id in range(0, 1000):
                scu.send(f"../data/data{id}", id)
                print(f"file sent: {id}", end="\r")

            # parallel
            # threads = []
            # for id in range(0, 1000):
            #     threads.append(threading.Thread(target = scu.send(f"../data/data{id}", id)))
            #     threads[-1].setDaemon(True)
            #     threads[-1].start()

            # for th in threads:
            #     th.join()
        except Exception as e:
            print(e)
            scu.drop() # We do not think its required, but just to make sure.

    elif sys.argv[1] == "receiver":
        # TODO
        scu = SCU(mtu = 1500)
        scu.bind_as_receiver(receiver_address = (RECEIVER_ADDRESS, PORT))
        for i in range(0, 1000):
            filedata = scu.recv()
            utils.write_file(f"../dataReceive/data{i}", filedata)
            print(f"file received: {i}", end="\r")
    else:
        print("Usage: python3 main.py sender|receiver")

if __name__ == '__main__':
    main()
