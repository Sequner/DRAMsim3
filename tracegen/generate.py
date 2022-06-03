from math import ceil
import numpy as np
import argparse

def main(args):
        
    file = open(args.output, 'w+')
    W_s = args.tREFW*(1-args.tRFC/args.tREFI)
    W_clock = W_s * args.clock
    W = W_s/args.tRC

    nvictims = args.nvictims if args.nvictims<W/args.T else ceil(W/args.T)
    length = W_clock*args.trace_len_in_W 
    local_clock = 0

    rowbits = np.log2(args.nrows)

    if(args.malicious):
        victims = generateVictims(nvictims, args.nrows, rowbits)
        s = gethexaddress(victims[1], rowbits)
        
        while(local_clock<length):
            for i in range(len(victims)):
                s = gethexaddress(victims[i]-1, rowbits)
                line = s + " " + "READ" + " " + str(local_clock) + "\n"
                file.write(line)
                local_clock += np.random.randint(10,100)
            
            for i in range((len(victims))):
                s = gethexaddress(victims[i]+1, rowbits)
                line = s + " " + "READ" + " " + str(local_clock) + "\n"
                file.write(line)
                local_clock += np.random.randint(10,100)

        file.close()


            

def generateVictims(nvictims:int, numrows:int, rowbits: int):
    victims = np.random.randint(1, numrows-1, size=nvictims)
    prologue = "X"*int((32-np.log2(numrows))/4)
    
    print("Victim rows are: ")
    for i in range(len(victims)):
        print('0x{0:0{1}X}'.format(victims[i], int(rowbits/4)) + prologue)

    return victims

def gethexaddress(row: int, rowbits: int):
    prologue_bytes = int((32-rowbits)/4)
    prologue = np.random.randint(0, 65536)
    row_hex = '0x{0:0{1}X}'.format(row, int(rowbits/4))
    prologue_hex = '{0:0{1}X}'.format(prologue,prologue_bytes)

    return row_hex+prologue_hex
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--malicious', type=bool, required=True)
    parser.add_argument('--clock', type=int, default=3200e6)
    parser.add_argument('--trace_len_in_W', type=int, default=1)
    parser.add_argument('--T', type=int, default = 50e3)
    parser.add_argument('--nrows', type=int, default=65536)
    parser.add_argument('--tRFC', type=int, default=560)
    parser.add_argument('--tREFI', type=int, default=12480)
    parser.add_argument('--tRC', type=int, default = 30e-9)
    parser.add_argument('--tREFW', type=int, default=64e-3)
    parser.add_argument('--nvictims', type=int, default=1)
    parser.add_argument('--output', type=str, default='./trace.txt')
    args = parser.parse_args()

    main(args)

    