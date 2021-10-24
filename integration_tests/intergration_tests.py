#
# This is a an integration testing framework that can be used to check your
# implementation against some of the typical scenarios. You don't have to
# implement your solution in Python, but nevertheless you're encouraged to use
# this framework to prove your solution works as expected. In order to do that,
# your solution has to be comparible with this framework in two aspects:
# 1. command-line arguments -- framework should be able to pass arguments to
#   your implementation and run it
# 2. client side file structure should mirror server side file structure
# The framework contains a simple and inefficient implementation. It's given
# for reference purposes only to demonstrate how to run the framework and that
# it's possible to pass all the tests.
#
# Install dependencies:
# pip3 install --user pytest
#
# How to run:
# mkdir -p /tmp/dropbox/client
# mkdir -p /tmp/dropbox/server
#
# The following environment variables are used to run a client & a server.
# export CLIENT_CMD='python3 -c "import test_homework as th; th.client()" -- hey_client /tmp/dropbox/client /tmp/dropbox/server'
# export SERVER_CMD='python3 -c "import test_homework as th; th.server()" -- hey_client /tmp/dropbox/client /tmp/dropbox/server'
#
# Verbose, with stdout, filter by test name
# pytest -vv -s . -k 'test_some_name'
# Quiet, show summary in the end
# pytest -q -rapP
# Verbose, with stdout, show summary in the end
# pytest -s -vv -q -rapP
#
from contextlib import closing
from glob import glob
from hashlib import md5
from itertools import chain
from os import environ, getpgid, killpg, mkdir, remove, rename, setsid, walk, listdir, unlink
from os.path import exists, getsize, isfile, join, sep, islink, isdir
from shutil import copytree, rmtree
from signal import SIGKILL, SIGTERM
from socket import socket, AF_INET, SOCK_STREAM, SOL_SOCKET, SO_REUSEADDR
from subprocess import Popen, TimeoutExpired
from sys import argv, stderr, stdout
from time import sleep
from unittest import TestCase

ASSERT_TIMEOUT = 20.0
ASSERT_STEP = 1.0
SHUTDOWN_TIMEOUT = 10.0

CLIENT_KEY = "abc"

SERVER_PATH = "../build/" + CLIENT_KEY
CLIENT_PATH = "../build/cl_dir"


def find_free_port():
    with closing(socket(AF_INET, SOCK_STREAM)) as s:
        s.bind(("", 0))
        s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
        return s.getsockname()[1]


def spit(filename, data):
    """Save data into the given filename."""
    with open(filename, "w") as file_:
        file_.write(data)


def reset_path(path):
    """Remove directory recursively and recreate again (empty)."""
    if exists(path):
        rmtree(path)
    mkdir(path)


def wipe_path(path):
    """Similar to reset_path, but walks the tree and removes each file/dir individually."""
    for filename in listdir(path):
        full = join(path, filename)
        if isfile(full) or islink(full):
            unlink(full)
        elif isdir(full):
            rmtree(full)


def sync_paths(source_path, dest_path):
    """Sync paths so that they contain exactly the same set of files."""
    files = chain(glob(join(dest_path, "*")), glob(join(dest_path, ".*")))
    for filename in files:
        if isfile(filename):
            remove(filename)
        else:
            rmtree(filename)
    if exists(dest_path):
        rmtree(dest_path)
    copytree(source_path, dest_path)


def get_md5(filename):
    if not isfile(filename):
        return "0"
    hash_md5 = md5()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def path_content_to_string(path):
    """Convert contents of a directory recursively into a string for easier comparison."""
    lines = []
    prefix_len = len(path + sep)
    for root, dirs, files in walk(path):
        for dir_ in dirs:
            full_path = join(root, dir_)
            relative_path = full_path[prefix_len:]
            size = 0
            type_ = "dir"
            hash_ = "0"
            line = "{},{},{},{}".format(relative_path, type_, size, hash_)
            lines.append(line)

        for filename in files:
            full_path = join(root, filename)
            relative_path = full_path[prefix_len:]
            size = getsize(full_path)
            type_ = "file" if isfile(full_path) else "dir"
            hash_ = get_md5(full_path)
            line = "{},{},{},{}".format(relative_path, type_, size, hash_)
            lines.append(line)

    lines = sorted(lines)
    return "\n".join(lines)


def assert_paths_in_sync(path1, path2, timeout=ASSERT_TIMEOUT, step=ASSERT_STEP):
    current_time = 0
    assert current_time < timeout, "we should always go around the loop at least once"
    while current_time < timeout:
        contents1 = path_content_to_string(path1)
        contents2 = path_content_to_string(path2)
        if contents1 == contents2:
            return
        sleep(step)
        current_time += step
    assert contents1 == contents2


def client():
    """Dumbest reference implementation of a client that copies files recursively."""
    print("CLIENT STARTED", argv)
    client_path = argv[-2]
    server_path = argv[-1]
    sync_paths(client_path, server_path)
    try:
        while True:
            if path_content_to_string(client_path) != path_content_to_string(server_path):
                print("Client syncing...")
                sync_paths(client_path, server_path)
            print("Client sleeping 1 second")
            sleep(1.0)
    finally:
        print("CLIENT DONE", argv)


def server():
    """Server reference implementation, does nothing, just for demo purposes."""
    print("SERVER STARTED", argv)
    try:
        while True:
            print("Server sleeping 1 second")
            sleep(1.0)
    finally:
        print("SERVER DONE", argv)


class Process:
    def __init__(self, cmd_line):
        print("Starting ", cmd_line)
        self._process = Popen(
            cmd_line, shell=True, preexec_fn=setsid, stdout=stdout, stderr=stderr
        )

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.shutdown()

    def shutdown(self):
        killpg(getpgid(self._process.pid), SIGTERM)
        try:
            self._process.wait(SHUTDOWN_TIMEOUT)
        except TimeoutExpired:
            killpg(getpgid(self._process.pid), SIGKILL)
        sleep(2.0)


class TestBasic(TestCase):
    def setUp(self):
        self.spath = SERVER_PATH
        self.cpath = CLIENT_PATH
        reset_path(self.spath)
        reset_path(self.cpath)
        port = find_free_port()
        self.server_cmd = environ["SERVER_CMD"].format(port=port, path=self.spath)
        self.client_cmd = environ["CLIENT_CMD"].format(port=port, path=self.cpath)
        print(self.server_cmd)
        print(self.client_cmd)
        self.server_process = Process(self.server_cmd)
        sleep(1.0)
        self.client_process = Process(self.client_cmd)
        sleep(1.0)
        assert_paths_in_sync(self.cpath, self.spath)

    def tearDown(self):
        self.server_process.shutdown()
        self.client_process.shutdown()

    def test_add_single_file(self):
        spit(join(self.cpath, "newfile.txt"), "contents")
        assert_paths_in_sync(self.cpath, self.spath)

    def test_single_file_completely_changes_3_times(self):
        spit(join(self.cpath, "newfile.txt"), "contents")
        assert_paths_in_sync(self.cpath, self.spath)

        spit(join(self.cpath, "newfile.txt"), "contents more")
        assert_paths_in_sync(self.cpath, self.spath)

        spit(join(self.cpath, "newfile.txt"), "beginning contents more")
        assert_paths_in_sync(self.cpath, self.spath)

        spit(join(self.cpath, "newfile.txt"), "new content")
        assert_paths_in_sync(self.cpath, self.spath)

    def test_single_file_change_and_remove(self):
        spit(join(self.cpath, "newfile.txt"), "contents")
        assert_paths_in_sync(self.cpath, self.spath)

        remove(join(self.cpath, "newfile.txt"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_add_empty_dir(self):
        mkdir(join(self.cpath, "newemptydir"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_add_and_remove_empty_dir(self):
        mkdir(join(self.cpath, "newemptydir"))
        assert_paths_in_sync(self.cpath, self.spath)

        rmtree(join(self.cpath, "newemptydir"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_3_new_files_1mb_each_add_instantly(self):
        spit(join(self.cpath, "file1.txt"), "*" * 10 ** 6)
        spit(join(self.cpath, "file2.txt"), "*" * 10 ** 6)
        spit(join(self.cpath, "file3.txt"), "*" * 10 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_one_dir_and_file_with_spaces(self):
        spit(join(self.cpath, "new file.txt"), "contents")
        mkdir(join(self.cpath, "new folder"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_one_dir_and_file_with_spaces_then_remove(self):
        spit(join(self.cpath, "new file.txt"), "contents")
        mkdir(join(self.cpath, "new folder"))
        assert_paths_in_sync(self.cpath, self.spath)

        rmtree(join(self.cpath, "new folder"))
        remove(join(self.cpath, "new file.txt"))
        assert_paths_in_sync(self.cpath, self.spath)
    
    def test_dir_with_nested_dir_and_files_within(self):
        mkdir(join(self.cpath, "test_dir"))
        mkdir(join(self.cpath, "test_dir", "nested_dir"))
        spit(join(self.cpath, "test_dir", "nested_dir", "file.txt"), "contents")
        mkdir(join(self.cpath, "test_dir", "nested_empty_dir"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_file_rename(self):
        spit(join(self.cpath, "file.txt"), "contents")
        rename(join(self.cpath, "file.txt"), join(self.cpath, "renamed.txt"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_dir_rename(self):
        mkdir(join(self.cpath, "test_dir"))
        rename(join(self.cpath, "test_dir"), join(self.cpath, "renamed_dir"))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_3_new_files_1mb_each_add_with_delay(self):
        spit(join(self.cpath, "file1.txt"), "*" * 10 ** 6)
        sleep(1.0)
        spit(join(self.cpath, "file2.txt"), "*" * 10 ** 6)
        sleep(1.0)
        spit(join(self.cpath, "file3.txt"), "*" * 10 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_single_file_change_1_byte_beginning(self):
        spit(join(self.cpath, "file1.txt"), "0" + "*" * 10 ** 6)
        sleep(1.0)
        assert_paths_in_sync(self.cpath, self.spath)

        spit(join(self.cpath, "file1.txt"), "1" + "*" * 10 ** 6)
        sleep(1.0)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_1_empty_file(self):
        spit(join(self.cpath, "file1.txt"), "")
        assert_paths_in_sync(self.cpath, self.spath)

    def test_3_empty_files_add_instantly(self):
        spit(join(self.cpath, "file1.txt"), "")
        spit(join(self.cpath, "file2.txt"), "")
        spit(join(self.cpath, "file3.txt"), "")
        assert_paths_in_sync(self.cpath, self.spath)

    def test_3_empty_files_add_with_delay(self):
        spit(join(self.cpath, "file1.txt"), "")
        sleep(1.0)
        spit(join(self.cpath, "file2.txt"), "")
        sleep(1.0)
        spit(join(self.cpath, "file3.txt"), "")
        assert_paths_in_sync(self.cpath, self.spath)

    def test_1_file_grows_twice_with_delay(self):
        spit(join(self.cpath, "file1.txt"), "*" * 10 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)
        sleep(1.0)
        spit(join(self.cpath, "file1.txt"), "*" * 20 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_1_file_shrinks_twice_with_delay(self):
        spit(join(self.cpath, "file1.txt"), "*" * 20 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)
        sleep(1.0)
        spit(join(self.cpath, "file1.txt"), "*" * 10 ** 6)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_many_small_files(self):
        for i in range(0, 1000):
            spit(join(self.cpath, "file_%i.txt" % (i,)), "contents_%i" % (i,))
        assert_paths_in_sync(self.cpath, self.spath)

    def test_file_to_empty_dirs_and_back(self):
        for i in range(0, 10):
            spit(join(self.cpath, "file_%i" % (i,)), "contents_%i" % (i,))
        assert_paths_in_sync(self.cpath, self.spath)

        wipe_path(self.cpath)
        for i in range(0, 10):
            mkdir(join(self.cpath, "file_%i" % (i,)))
        assert_paths_in_sync(self.cpath, self.spath)

        wipe_path(self.cpath)

        for i in range(0, 10):
            spit(join(self.cpath, "file_%i" % (i,)), "contents_%i" % (i,))

        assert_paths_in_sync(self.cpath, self.spath)

class TestInitialSync(TestCase):
    def setUp(self):
        self.spath = SERVER_PATH
        self.cpath = CLIENT_PATH
        reset_path(self.spath)
        reset_path(self.cpath)
        port = find_free_port()
        self.server_cmd = environ["SERVER_CMD"].format(port=port, path=self.spath)
        self.client_cmd = environ["CLIENT_CMD"].format(port=port, path=self.cpath)
        print(self.server_cmd)
        print(self.client_cmd)

    def tearDown(self):
        self.server_process.shutdown()
        self.client_process.shutdown()

    def test_one_file(self):
        spit(join(self.cpath, "newfile.txt"), "contents")

        self.server_process = Process(self.server_cmd)
        sleep(1.0)
        self.client_process = Process(self.client_cmd)
        sleep(1.0)
        assert_paths_in_sync(self.cpath, self.spath)

    def test_file_and_empty_dir(self):
        spit(join(self.cpath, "newfile.txt"), "contents")
        mkdir(join(self.cpath, "newemptydir"))

        self.server_process = Process(self.server_cmd)
        sleep(1.0)
        self.client_process = Process(self.client_cmd)
        sleep(1.0)
        assert_paths_in_sync(self.cpath, self.spath)
