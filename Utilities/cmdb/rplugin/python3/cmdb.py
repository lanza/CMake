#!/usr/bin/env python3

import atexit
import socket
import pynvim
from enum import Enum
import threading
import os
import sys

from typing import Optional
import time

import pysnooper

if not os.path.exists("/tmp/cmdb"):
    os.mkdir("/tmp/cmdb")


def unused(_):
    pass


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
    return str(data)


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
    scHandshake = 2
    csHandshake = 3
    ciUpdateFrame = 33
    ciVariableEvaluation = 34
    ciStepOver = 35
    ciStepOut = 36
    ciStepIn = 37
    ciContnue = 38

    # 0   Invalid
    # 1   scError
    # 2   scHandshake
    # 3   csHandshake
    # 4   scTargetStopped
    # 5   scTargetRunning
    # 6   csContinue
    # 7   csStepIn
    # 8   csStepOut
    # 9   csStepOver
    # 10  csBreakIn
    # 11  csSetNextStatement
    # 12  csTerminate
    # 13  csDetach
    # 14  csCreateExpression
    # 15  scExpressionCreated
    # 16  csQueryExpressionChildren
    # 17  scExpressionChildrenQueried
    # 18  csSetExpressionValue
    # 19  scExpressionUpdated
    # 20  BeforeFirstBreakpointRelatedCommand
    # 21  csCreateBreakpoint
    # 22  csCreateFunctionBreakpoint
    # 23  csCreateDomainSpecificBreakpoint
    # 24  scBreakpointCreated
    # 25  csDeleteBreakpoint
    # 26  csUpdateBreakpoint
    # 27  csQueryBreakpoint
    # 28  scBreakpointQueried
    # 29  scBreakpointUpdated
    # 30  AfterLastBreakpointRelatedCommand
    # 31  scDebugMessage
    # 32  scTargetExited
    # 33  ciUpdateFrame


VIM = None

MARK = None


@pysnooper.snoop("/tmp/asynclog")
def adjust_line(*args):
    global MARK
    file = args[1]
    line = args[0]
    VIM.current.window = VIM.windows[0]
    VIM.command(f"e {file}")
    VIM.current.window.cursor = (line, 0)
    if MARK:
        VIM.command("sign unplace 2")
    MARK = (file, line)
    VIM.command(f"sign place 2 line={line} name=piet file={file}")
    VIM.current.window = VIM.windows[1]
    VIM.command("startinsert")


def echo_variable(*args):
    VIM.command(f'echom "{args[0]}"')


class VimCmdbListenerThread(threading.Thread):
    def __init__(self, s: socket.socket, v: pynvim.Nvim):
        threading.Thread.__init__(self)
        self.s = s
        self.v = v

    @pysnooper.snoop("/tmp/cmdb/CMDBVimListenerThread")
    def run(self):
        s = self.s
        vim = self.v
        s.settimeout(0.5)
        while True:
            try:
                data = s.recv(6)
                break
            except:
                continue
        t, data = read_uint32_at(data)
        size, data = read_uint32_at(data)
        data = s.recv(size)
        if t == IDEType.scHandshake.value:
            s.send(encode_unsigned(3) + encode_unsigned(0))
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

            if t == IDEType.ciUpdateFrame.value:
                file, data = read_string_at(data)
                line, data = read_uint32_at(data)
                vim.async_call(adjust_line, line, file)
            elif t == IDEType.ciVariableEvaluation.value:
                value, data = read_string_at(data)
                vim.async_call(echo_variable, value)


LISTENER_THREAD = None

@pynvim.plugin
class Main(object):
    def __init__(self, vim: pynvim.api.nvim.Nvim):
        global VIM
        VIM = vim
        self.vim = vim
        VIM.command("sign define piet text=>> texthl=Search")

    @pysnooper.snoop("/tmp/cmdb/CMDBListen")
    @pynvim.command("CMDBListen")
    def cmdb_listen(self):
        global LISTENER_THREAD
        try_count = 0
        while True:
            if try_count > 0:
                time.sleep(1)
            try:
                try_count += 1
                self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.socket.connect(("127.0.0.1", 6992))
            except:
                pass
            finally:
                break

        LISTENER_THREAD = VimCmdbListenerThread(self.socket, self.vim)
        LISTENER_THREAD.start()


    @pynvim.command("CMDBExpression", nargs="1")
    def cmdb_expression(self, *args):
        self.socket.send(encode_unsigned(len(args[0][0]) + 4))
        self.socket.send(encode_info(str(args[0][0])))

    # @pynvim.command("CMDBExpressionRange", range="")
    # def cmdb_expression_range(self, range):
    #     self.vim.current.line = f"Command with range: {range}"
    #     self.socket.send(encode_unsigned(len(range) + 4))
    #     self.socket.send(encode_info(str(range)))

    # @pynvim.command("CMDBStepOver")
    # def cmd_step_over(self):
    #     self.socket.send("idk")
