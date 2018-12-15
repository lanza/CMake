#!/usr/bin/env python3

import socket
import sys
import pynvim
from enum import Enum
import threading


def encode_unsigned(i: int) -> bytes:
    return i.to_bytes(4, byteorder="little")


def encode_string(s: str) -> bytes:
    length = len(s)
    data = encode_unsigned(length) + s.encode("ascii")
    return data


def encode_info(*args):
    data = bytes()
    for arg in args:
        t = type(arg)
        if t is int:
            data += encode_unsigned(arg)
        elif t is str:
            data += encode_string(arg)
        else:
            print(f"Bad encode_info: {t}")

    return data


def uint32_from_bytes(data):
    return int.from_bytes(data, byteorder="little")


def string_from_bytes(data):
    return str(bytes)


def read_string_at(data):
    length = uint32_from_bytes(data[0:3])
    if length > 0:
        string = data[4 : (4 + length)].decode("ascii")
        return (string, data[(4 + length) :])
    else:
        return ("", data[4:])


def read_uint32_at(data):
    return (uint32_from_bytes(data[0:3]), data[4:])


class IDEType(Enum):
    Error = 0
    FileAndLocation = 1
    VariableEvaluation = 2


VIM = None


def adjust_line(*args):
    VIM.command(f"e {args[1]}")
    VIM.command(f"normal {args[0]}gg")


def echo_variable(*args):
    VIM.command(f'echom "{args[0]}"')


class VimCmdbThread(threading.Thread):
    def __init__(self, s: socket.socket, v):
        threading.Thread.__init__(self)
        self.s = s
        self.v = v

    def run(self):
        s = self.s
        vim = self.v
        s.settimeout(0.5)
        while True:
            try:
                data = s.recv(8)
            except:
                continue
            if len(data) == 0:
                s.close()
                return
            t, data = read_uint32_at(data)
            size, data = read_uint32_at(data)
            data = s.recv(size)

            if t == IDEType.FileAndLocation.value:
                line, data = read_uint32_at(data)
                file, data = read_string_at(data)
                vim.async_call(adjust_line, line, file)
            elif t == IDEType.VariableEvaluation.value:
                value, data = read_string_at(data)
                vim.async_call(echo_variable, value)

THREAD = None
@pynvim.plugin
class Main(object):
    def __init__(self, vim: pynvim.api.nvim.Nvim):
        global VIM
        VIM = vim
        self.vim = vim

    @pynvim.command("CMDBListen")
    def cmdb_listen(self):
        global THREAD
        # self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.s.connect(("127.0.0.1", int(args[0][0])))
        self.s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.s.connect("/tmp/cmdb/vim-cmdb")
        THREAD = VimCmdbThread(self.s, self.vim)
        THREAD.start()

    @pynvim.command("CMDBExpression", nargs="1")
    def cmdb_expression(self, *args):
        self.s.send(encode_unsigned(len(args[0][0]) + 4))
        self.s.send(encode_info(str(args[0][0])))

    @pynvim.command("CMDBExpressionRange", range='')
    def cmdb_expression_range(self, range):
        self.vim.current.line = (f'Command with range: {range}')
        self.s.send(encode_unsigned(len(range) + 4))
        self.s.send(encode_info(str(range)))
