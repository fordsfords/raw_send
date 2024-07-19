import datetime
import getopt
import re
import os
import subprocess
import sys


def raw_sendem( hexdata ):
    rsend = globals()['raw_send_path']
    rsargs = globals()['raw_send_args']

    if globals()['outfile'] != "":
        print( rsend, " ", rsargs, " ", hexdata, file=globals()['fout'])

    args = [rsend, rsargs, hexdata ]

    subprocess.run(args)


def processArgs( argsIn ):

    # default values
    globals()['outfile'] = "raw_send.sh"
    globals()['pktfile'] = "packet-IPv4.txt"
    globals()['raw_send_path'] = "./raw_send"
    globals()['raw_send_args'] = "eth0"

    print( "In processArgs: ", argsIn[1:] )

    try:
        opts, args = getopt.getopt(argsIn[1:], 'hA:o:p:P:')
    except getopt.GetoptError:
        print_help( argsIn[0])
        sys.exit(2)

    print( "In processArgs: ", args )
    for opt, arg in opts:
        print( "in processArgs: ", opt, ", instance=", type(opt) )

        if len(opt) == 0:
            continue

        if opt == "-h":
            print_help( argsIn[0])
            sys.exit()
        elif opt == "-o":
            globals()['outfile'] = arg
            continue
        elif opt == "-p":
            globals()['pktfile'] = arg
            print( "pktfile is ", arg )
            continue
        elif opt == '-P':
            globals()['raw_send_path'] = arg
            continue
        elif opt == "-A":
            globals()['raw_send_args'] = arg
            continue

    # now check that we have valid inputs
    if globals()['outfile'] != "":
        try:
            globals()['fout'] = open( globals()['outfile'], "w")
        except:
            print("Invalid output file ", globals()['outfile'])
            sys.exit(2)

    if globals()['pktfile'] != "":
        try:
            globals()['fin'] = open(globals()['pktfile'], "r")
        except:
            print("Invalid packet file ", globals()['pktfile'])
            sys.exit(2)

    if globals()['raw_send_path'] != "":
        if not ( os.path.isfile( globals()['raw_send_path']) and \
           os.access( globals()['raw_send_path'], os.X_OK ) ):
            print("Invalid path to raw_send")
            sys.exit(2)

def print_help( procName):
    print("Usage: ", procName, ' [-h] [-o output file] -p <packet_file> [-P <full_path_to_raw_send] [-A "<args to raw_send>"]' )

def main(argv):
    processArgs(argv)

    fin = globals()['fin']

    pkthex = str("")

    for linein in fin:
        print ( linein )
        # blank line is end of packet
        if linein == "":
            if pkthex == "":
                continue
            else:
                ts = ts + 0.020
                raw_sendem( pkthex, ts )
                pkthex = str("")

        # odd, the if above should catch all breaks between
        # packets, so code below should not really be hit
        # However, I added it after section above was getting missed
        elif re.match( "0000  ", linein ):
            if pkthex == "":
                continue
            else:
                raw_sendem( pkthex )
                pkthex = str("")

                #begin new packet
                newdata = linein[6:53]
                append_data = newdata.replace(" ", "")
                # print( append_data )
                pkthex = append_data

        else:
            # add to current packet
            newdata = linein[6:53]
            append_data = newdata.replace(" ", "")
            #print( append_data )
            pkthex = pkthex + append_data



if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.argv.append("/home/sclutter/files/SRs/04653453/repo_test_packets/packet-IPv4.txt")
    main(sys.argv)
