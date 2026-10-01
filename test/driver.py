#!/usr/bin/env python3
# One engine process per call. The binary defaults to build/neptune.

import os
from pathlib import Path
from subprocess import PIPE, Popen, TimeoutExpired


def engine_path():
    override = os.environ.get('NEPTUNE')
    if override:
        return override
    root = Path(__file__).resolve().parent.parent
    return str(root / 'build' / 'neptune')


def _finish(process, out, err):
    if process.returncode != 0:
        raise Exception('Engine exited %s: %s' % (
            process.returncode, err.decode('ascii', 'replace')))
    if err:
        raise Exception(err.decode('ascii', 'replace'))
    text = out.decode('ascii').strip()
    if not text:
        return []
    return text.split('\n')


def run(commands, timeout=300):
    process = Popen([engine_path()], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    script = '\n'.join(commands) + '\nquit\n'
    try:
        out, err = process.communicate(input=script.encode('ascii'), timeout=timeout)
    except TimeoutExpired:
        process.kill()
        process.communicate()
        raise Exception('Engine timed out')
    return _finish(process, out, err)


def run_until_eof(commands, timeout=5):
    process = Popen([engine_path()], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    script = '\n'.join(commands) + '\n'
    try:
        out, err = process.communicate(input=script.encode('ascii'), timeout=timeout)
    except TimeoutExpired:
        process.kill()
        process.communicate()
        raise Exception('Engine did not exit on EOF')
    return _finish(process, out, err)
