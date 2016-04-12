import parse
import json
import argparse


parser = argparse.ArgumentParser()
parser.add_argument('logFile', help='log filename')
parser.add_argument('-o', '--output', help='Raw output log operations (CSV format)')
parser.add_argument('-s', '--summary', action='store_true', help='Summary of operations')
args = parser.parse_args()


def delta_ms(start, finish):
    delta = finish - start
    return delta.seconds * 1000.0 + delta.microseconds / 1000.0

class LogFile:

    def __init__(self, filename, output, summary):
        self.filename = filename
        self.output = open(output, "w+") if output else None
        self.summary = summary
        self.operations = {}
        self.last_getblocktemplate = None
        self.jobs = {}

    def parse(self):
        """Parse log file"""
        with open(self.filename, "r") as f:
            # A logged line can contain a '\n', so we join them before parsing
            prevline = ""
            for line in f:
                if line[0] != "[":
                    prevline += line
                else:
                    if prevline:
                        self.parseline(prevline)
                    prevline = line
            if prevline:
                self.parseline(prevline)
            #self.flushlines()

    def parseline(self, line):
        if line.find("ROOTSTOCK:") < 0: # Ignore lines that were not logged by us
            return None

        # Log line format is [time] ROOTSTOCK: <operation>: <data>
        result = parse.parse("[{:ti}] {}: {}: {}", line)

        # Drop ill formed lines
        if (result is None) or (len(result.fixed) != 4) or (result.fixed[1] != 'ROOTSTOCK'):
            print("Failed to parse: |{}|".format(line))
            return None

        time = result.fixed[0]
        operation = result.fixed[2]
        data = result.fixed[3]

        # Interpret logged operations
        if operation == 'json_rpc_call':
            rpc_call = parse.parse("{:x}, {}", data)
            if (rpc_call.fixed is None) or (len(rpc_call.fixed) != 2):
                print("Failed to parse: |{}|".format(line))
                return None
            call_id = rpc_call.fixed[0]
            call = rpc_call.fixed[1]

            return self.process('json_rpc_call', call_id, time, call)

        elif operation == 'json_rpc_reply':
            rpc_reply = parse.parse("{:x}, {}", data)
            if (rpc_reply.fixed is None) or (len(rpc_reply.fixed) != 2):
                print("Failed to parse: |{}|".format(line))
                return None
            call_id = rpc_reply.fixed[0]
            reply = rpc_reply.fixed[1]

            return self.process('json_rpc_reply', call_id, time, reply)

        elif operation == "send_client_send":
            send_client = parse.parse("{:d}, {:x}, {}", data)
            client_id = send_client.fixed[0]
            send_id = send_client.fixed[1]
            send = send_client.fixed[2]

            return self.process('send_client_send', send_id, time, send, client_id)

        elif operation == "send_client_complete":
            send_client = parse.parse("{:d}, {:x}", data)
            client_id = send_client.fixed[0]
            send_id = send_client.fixed[1]

            return self.process('send_client_complete', send_id, time, '', client_id)

        elif operation == "parse_client_msg":
            client_message = parse.parse("{:d}, {}", data)
            client_id = client_message.fixed[0]
            message = client_message.fixed[1]
            return self.process('parse_client_msg', client_id, time, message)

        elif operation == "getblocktemplate":
            message = parse.parse("{:ti}, {:ti}, {}", data)
            start_time = message.fixed[0]
            finish_time = message.fixed[1]
            work_id = message.fixed[2]
            return self.process('getblocktemplate', work_id, start_time, '', finish_time)

        elif operation == "blocksolve":
            message = parse.parse("{}, {}, {}, {}", data)
            jobid = message.fixed[0]
            nonce = message.fixed[1]
            nonce2 = message.fixed[2]
            blockhash = message.fixed[3]
            return self.process('blocksolve', jobid, time, (jobid, nonce, nonce2, blockhash))

        elif operation == "submitblock":
            message = parse.parse("{:ti}, {:ti}, {}:{}", data)
            start_time = message.fixed[0]
            finish_time = message.fixed[1]
            result = message.fixed[2]
            blockhash = message.fixed[3]
            return self.process('submitblock', blockhash, start_time, result, finish_time)

        print("Failed to parse: |{}|".format(line))
        return None

    # Process individual operations and map them to a high livel operation
    def process(self, operation, id, time, data, *args):
        if operation == 'json_rpc_call':
            # Before json-rpc call
            method = self.jsonrpc_method(data)
            self.operations[id] = (method, time, data)

        elif operation == 'json_rpc_reply':
            # After a successful json-rpc call
            if id in self.operations:
                method, start_time, call = self.operations[id]

                if method in ['submitblock']:
                    pass
                    # self.format(method, start_time, delta_ms(start_time, time))

                del self.operations[id]
            else:
                print("Error json_rpc_reply without json_rpc_call {} at {}".format(data, time))

        elif operation == 'send_client_send':
            # Before sending a message to a miner
            method = self.jsonrpc_method(data)
            self.operations[id] = (method, time, data)

        elif operation == 'send_client_complete':
            # After the message was sent to a miner
            if id in self.operations:
                client_id = args[0]
                method, start_time, call = self.operations[id]

                if method == 'mining.notify':
                    jobid = self.notify_jobid(self.operations[id][2])
                    self.format(method, start_time, delta_ms(start_time, time), ":".join([jobid, str(client_id)]))

                del self.operations[id]
            else:
                print("Error send_client_complete without send_client_send {} at {}".format(data, time))

        elif operation == 'parse_client_msg':
            # Message received from a miner
            method = self.jsonrpc_method(data)
            if method == 'mining.submit':
                jobid, nonce = self.submit_jobid(data)
                self.jobs[nonce] = (jobid, time, nonce)
                self.format(method, time, 0.0, ":".join([jobid,nonce]))

        elif operation == 'getblocktemplate':
            finish_time = args[0]
            self.format('getblocktemplate', time, delta_ms(time, finish_time), id)

        elif operation == 'blocksolve':
            jobid, nonce, nonce2, blockhash = data
            if nonce in self.jobs:
                sub_jobid, sub_time, sub_nonce = self.jobs[nonce]
                if jobid == sub_jobid:
                    del self.jobs[nonce]
                    self.jobs[blockhash] = (sub_jobid, sub_time, sub_nonce, blockhash)
                    self.format('blocksolve', time, 0.0, ":".join([jobid, nonce, blockhash]))
                else:
                    print("Error blocksolve without valid job: {}, {}, {}".format(nonce, jobid, time))
            else:
                print("Error blocksolve without valid job: {}, {}, {}".format(nonce, jobid, time))

        elif operation == 'submitblock':
            finish_time = args[0]
            self.format('submitblock', time, delta_ms(time, finish_time), id)

    def jsonrpc_method(self, data):
        """Recover the method from a json-rpc message"""
        try:
            call = json.loads(data)
            if 'method' not in call:
                return None
            return call['method']
        except json.decoder.JSONDecodeError as ex:
            if data.find('"method": "submitblock"') >= 0:
                return 'submitblock'
        except:
            raise

        return None

    def notify_jobid(self, data):
        """Recover the jobid for a mining.notify message"""
        try:
            message = json.loads(data)
            return message['params'][0]
        except:
            print(" Failed to parse '{}'".format(data))
            raise

    def submit_jobid(self, data):
        """Recover the jobid from a mining.submit message"""
        try:
            message = json.loads(data)
            return message['params'][1], message['params'][4]
        except:
            print(" Failed to parse '{}'".format(data))
            raise

    def format(self, method, start, duration, id=None):
        if self.output:
            self.output.write("{}, {}, {}, {}\n".format(method, start, duration, '' if id is None else id))
        if self.summary:
            self.add_operation(method, start, duration, id)
        #else:
        #    print("{}, {}, {}, {}".format(method, start, duration, '' if id is None else id))

    def add_operation(self, method, start, duration, id=None):

        if method == 'getblocktemplate':
            if self.last_getblocktemplate is not None:
                prev_start, prev_duration, prev_id, prev_clients = self.last_getblocktemplate
                last_client = delta_ms(prev_start, prev_clients[-1]) - prev_duration if len(prev_clients) > 0 else '--'
                print("getblocktemplate, {}, {}, {}, {}, {}".format(prev_start, prev_duration, prev_id, len(prev_clients), last_client))

            self.last_getblocktemplate = (start, duration, id, [])

        elif method == 'mining.notify':
            if self.last_getblocktemplate is not None:
                prev_start, prev_duration, prev_id, prev_clients = self.last_getblocktemplate
                if prev_id == id.split(":")[0]:
                    prev_clients += [start]
                    self.last_getblocktemplate = prev_start, prev_duration, prev_id, prev_clients
                else:
                    print("Error mining.notify without getblocktemplate: {}, {} ".format(start, id))
            else:
                print("Error mining.notify without getblocktemplate: {}, {}".format(start, id))

        elif method == 'submitblock':
            if id in self.jobs:
                sub_jobid, sub_time, sub_nonce, blockhash = self.jobs[id]
                print("submitblock, {}, {}, {}, 1, {}".format(sub_time, delta_ms(sub_time, start), sub_jobid, duration))
                del self.jobs[id]
            else:
                print("Error submitblock without valid job: {}, {}".format(id, start))


def main():
    logFile = LogFile(args.logFile, args.output, args.summary)
    logFile.parse()


if __name__ == "__main__":
    main()
