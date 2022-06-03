import numpy as np
import configparser
import argparse

dec2bin = lambda val, len: format(val, 'b').zfill(len)
bin2hex = lambda val, len: format(val, 'h').zfill(len)


def main(args):
    config = configparser.ConfigParser()    
    config.read(args.config)

    nrows = int(config['dram_structure']['rows'])
    ncols = int(config['dram_structure']['columns'])
    bl = int(config['dram_structure']['BL'])
    nbankgroups = int(config['dram_structure']['bankgroups'])
    nbanks = int(config['dram_structure']['banks_per_group'])
    nchannels = int(config['system']['channels']) 
    nranks = 2

    dontcare_bits = int(np.log2(ncols) - np.log2(bl) + np.log2(nbankgroups) + np.log2(nbanks) + np.log2(nranks) + np.log2(nchannels) + np.log2(int(config['system']['bus_width'])*bl/8))
    rowbits = 32-dontcare_bits

    nvictims = args.nvictims 


    file = open(args.output, 'w+')
    local_clock = 0
    

    
    victims = generateVictims(nvictims, rowbits)
    
    while(local_clock<args.trace_duration):
        for i in range(len(victims)):
            s = gethex(victims[i]-1, rowbits)
            line = s + " " + "READ" + " " + str(local_clock) + "\n"
            file.write(line)
            local_clock += np.random.randint(10,100)
        
        for i in range((len(victims))):
            s = gethex(victims[i]+1, rowbits)
            line = s + " " + "READ" + " " + str(local_clock) + "\n"
            file.write(line)
            local_clock += np.random.randint(10,100)

 

    file.close()
    

def generateVictims(nvictims:int, rowbits: int):
    victims = np.random.randint(1, (2**rowbits)-1, size=nvictims)

    print("Victim rows (in decimal) are: ")
    for i in range(len(victims)):
        print(victims[i])

    return victims

def gethex(row, rowbits):
    row_bin = dec2bin(row, rowbits)
    prologue = np.random.randint(0, (2**(32-rowbits))-1)
    prologue_bin = dec2bin(prologue, 32-rowbits)
    
    address_bin = row_bin + prologue_bin
    address_hex = '0x{:0{width}X}'.format(int(address_bin, 2), width=8)

    return address_hex   

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument('--config', type=str, required=True)
    parser.add_argument('--trace_duration', type=int, default=100e6)
    parser.add_argument('--nvictims', type=int, default=1)
    parser.add_argument('--output', type=str, default='./trace.txt')
    
    # parser.add_argument('--T', type=int, default = 50e3)
    
    # parser.add_argument('--tRC', type=int, default = 30e-9)
    # parser.add_argument('--tREFW', type=int, default=64e-3)

    
    args = parser.parse_args()

    main(args)

    