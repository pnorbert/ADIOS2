#!/usr/bin/env python3

import argparse
import glob
import sqlite3
import zlib
from datetime import datetime
from os import chdir, getcwd, remove, stat
from os.path import exists, isdir, expanduser
from re import sub
from socket import getfqdn
from time import time

# from adios2.adios2_campaign_manager import *

ADIOS_ACA_VERSION = "1.1"


def ReadConfig():
    path = expanduser("~/.config/adios2/campaign.cfg")
    try:
        with open(path) as f:
            lines = f.readlines()
            for line in lines:
                lst = line.split()
                if lst[0] == "campaignstorepath":
                    adios_campaign_store = expanduser(lst[1])
    except FileNotFoundError:
        adios_campaign_store = None
    return adios_campaign_store


def SetupArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command",
        help="Command: create/update/delete/info/list",
        choices=["create", "update", "delete", "info", "list"],
    )
    parser.add_argument(
        "campaign", help="Campaign name or path, with .aca or without", default=None, nargs="?"
    )
    parser.add_argument("--verbose", "-v", help="More verbosity", action="count", default=0)
    parser.add_argument(
        "--campaign_store", "-s", help="Path to local campaign store", default=None
    )
    parser.add_argument(
        "--hostname", "-n", help="Host name unique for hosts in a campaign", required=False
    )
    args = parser.parse_args()

    # default values
    args.update = False
    if args.campaign_store is None:
        args.campaign_store = ReadConfig()

    if args.campaign_store is not None:
        while args.campaign_store[-1] == "/":
            args.campaign_store = args.campaign_store[:-1]

    args.CampaignFileName = args.campaign
    if args.campaign is not None:
        if not args.campaign.endswith(".aca"):
            args.CampaignFileName += ".aca"
        if args.campaign_store is not None:
            args.CampaignFileName = args.campaign_store + "/" + args.CampaignFileName

    args.LocalCampaignDir = "adios-campaign/"

    if args.verbose > 0:
        print(f"# Verbosity = {args.verbose}")
        print(f"# Command = {args.command}")
        print(f"# Campaign File Name = {args.CampaignFileName}")
        print(f"# Campaign Store = {args.campaign_store}")
    return args


def CheckCampaignStore(args):
    if args.campaign_store is not None and not isdir(args.campaign_store):
        print("ERROR: Campaign directory " + args.campaign_store + " does not exist", flush=True)
        exit(1)


def CheckLocalCampaignDir(args):
    if not isdir(args.LocalCampaignDir):
        print(
            "ERROR: Shot campaign data '"
            + args.LocalCampaignDir
            + "' does not exist. Run this command where the code was executed.",
            flush=True,
        )
        exit(1)


def IsHDF5File(dataset: str):
    if not exists(dataset):
        return False
    answer = False
    HDF5Header = [137, 72, 68, 70, 13, 10, 26, 10]
    with open(dataset, "rb") as f:
        header = f.read(len(HDF5Header))
        if len(header) == len(HDF5Header):
            answer = True
            for i in range(len(HDF5Header)):
                if header[i] != HDF5Header[i]:
                    answer = False
    return answer


def IsADIOSDataset(dataset: str):
    if not isdir(dataset):
        return False
    if not exists(dataset + "/" + "md.idx"):
        return False
    if not exists(dataset + "/" + "data.0"):
        return False
    return True


def CompressFile(f):
    compObj = zlib.compressobj()
    compressed = bytearray()
    blocksize = 1073741824  # 1GB #1024*1048576
    len_orig = 0
    len_compressed = 0
    block = f.read(blocksize)
    while block:
        len_orig += len(block)
        cBlock = compObj.compress(block)
        compressed += cBlock
        len_compressed += len(cBlock)
        block = f.read(blocksize)
    cBlock = compObj.flush()
    compressed += cBlock
    len_compressed += len(cBlock)

    return compressed, len_orig, len_compressed


# def DecompressBuffer(buf: bytearray):
#    data = zlib.decompress(buf)
#    return data


def AddFileToArchive(args: dict, filename: str, cur: sqlite3.Cursor, dsID: int):
    compressed = 1
    try:
        f = open(filename, "rb")
        compressed_data, len_orig, len_compressed = CompressFile(f)

    except IOError:
        print(f"ERROR While reading file {filename}")
        return

    statres = stat(filename)
    ct = int(statres.st_ctime_ns / 1000)

    cur.execute(
        "insert into bpfile values (?, ?, ?, ?, ?, ?, ?)",
        (dsID, filename, compressed, len_orig, len_compressed, ct, compressed_data),
    )
    con.commit()

    # test
    # if (filename == "dataAll.bp/md.0"):
    #    data = DecompressBuffer(compressed_data)
    #    of = open("dataAll.bp-md.0", "wb")
    #    of.write(data)
    #    of.close()


def AddDatasetToArchive(args: dict, dataset: str, cur: sqlite3.Cursor, hostID: int, dirID: int):
    if IsADIOSDataset(dataset):
        print(f"Add dataset {dataset} to archive")
        statres = stat(dataset)
        ct = int(statres.st_ctime_ns / 1000)
        curDS = cur.execute(
            "insert into bpdataset values (?, ?, ?, ?)", (hostID, dirID, dataset, ct)
        )
        dsID = curDS.lastrowid
        cwd = getcwd()
        chdir(dataset)
        mdFileList = glob.glob("*md.*")
        profileList = glob.glob("profiling.json")
        files = mdFileList + profileList
        for f in files:
            AddFileToArchive(args, f, cur, dsID)
        chdir(cwd)
    elif IsHDF5File(dataset):
        print(f"Add HDF5 file {dataset} to archive")
        statres = stat(dataset)
        ct = int(statres.st_ctime_ns / 1000)
        curDS = cur.execute(
            "insert into bpdataset values (?, ?, ?, ?)", (hostID, dirID, dataset, ct)
        )
    else:
        print(f"WARNING: Dataset {dataset} is not an ADIOS dataset nor an HDF5 file. Skip")


def ProcessDBFile(args: dict, jsonlist: list, cur: sqlite3.Cursor, hostID: int, dirID: int):
    for entry in jsonlist:
        # print(f"Process entry {entry}:")
        if isinstance(entry, dict):
            if "name" in entry:
                AddDatasetToArchive(args, entry["name"], cur, hostID, dirID)
        else:
            print(f"WARNING: your object is not a dictionary, skip : {entry}")


def GetHostName():
    host = getfqdn()
    if host.startswith("login"):
        host = sub("^login[0-9]*\\.", "", host)
    if host.startswith("batch"):
        host = sub("^batch[0-9]*\\.", "", host)
    shorthost = host.split(".")[0]
    return host, shorthost


def AddHostName(longHostName, shortHostName):
    res = cur.execute('select rowid from host where hostname = "' + shortHostName + '"')
    row = res.fetchone()
    if row is not None:
        hostID = row[0]
        print(f"Found host {shortHostName} in database, rowid = {hostID}")
    else:
        curHost = cur.execute("insert into host values (?, ?)", (shortHostName, longHostName))
        hostID = curHost.lastrowid
        print(f"Inserted host {shortHostName} into database, rowid = {hostID}")
    return hostID


def Info(args: dict, cur: sqlite3.Cursor):
    res = cur.execute("select id, name, version, ctime from info")
    info = res.fetchone()
    t = datetime.fromtimestamp(float(info[3]))
    print(f"{info[1]}, version {info[2]}, created on {t}")

    res = cur.execute("select rowid, hostname, longhostname from host")
    hosts = res.fetchall()
    for host in hosts:
        print(f"hostname = {host[1]}   longhostname = {host[2]}")
        res2 = cur.execute(
            'select rowid, name from directory where hostid = "' + str(host[0]) + '"'
        )
        dirs = res2.fetchall()
        for dir in dirs:
            print(f"    dir = {dir[1]}")
            res3 = cur.execute(
                'select rowid, name, ctime from bpdataset where hostid = "'
                + str(host[0])
                + '" and dirid = "'
                + str(dir[0])
                + '"'
            )
            bpdatasets = res3.fetchall()
            for bpdataset in bpdatasets:
                t = datetime.fromtimestamp(float(bpdataset[2] / 1000000))
                print(f"        dataset = {bpdataset[1]}     created on {t}")


def Update(args: dict, cur: sqlite3.Cursor):
    longHostName, shortHostName = GetHostName()
    if args.hostname is not None:
        shortHostName = args.hostname

    hostID = AddHostName(longHostName, shortHostName)

    rootdir = getcwd()
    # curHost = cur.execute('insert into host values (?, ?)',
    #                      (shortHostName, longHostName))
    # hostID = curHost.lastrowid

    curDir = cur.execute("insert or replace into directory values (?, ?)", (hostID, rootdir))
    dirID = curDir.lastrowid
    con.commit()

    print(f"dbFileList = {dbFileList}")
    db_list = MergeDBFiles(dbFileList)

    print(f"Merged db list = {db_list}")
    ProcessDBFile(args, db_list, cur, hostID, dirID)

    con.commit()


def Create(args: dict, cur: sqlite3.Cursor):
    epoch = time()
    cur.execute("create table info(id TEXT, name TEXT, version TEXT, ctime INT)")
    cur.execute(
        "insert into info values (?, ?, ?, ?)",
        ("ACA", "ADIOS Campaign Archive", ADIOS_ACA_VERSION, epoch),
    )
    cur.execute("create table host" + "(hostname TEXT PRIMARY KEY, longhostname TEXT)")
    cur.execute("create table directory" + "(hostid INT, name TEXT, PRIMARY KEY (hostid, name))")
    cur.execute(
        "create table bpdataset"
        + "(hostid INT, dirid INT, name TEXT, ctime INT"
        + ", PRIMARY KEY (hostid, dirid, name))"
    )
    cur.execute(
        "create table bpfile"
        + "(bpdatasetid INT, name TEXT, compression INT, lenorig INT"
        + ", lencompressed INT, ctime INT, data BLOB"
        + ", PRIMARY KEY (bpdatasetid, name))"
    )
    cur.execute(
        "create table step"
        + "(bpdatasetid INT, enginestep INT, physstep INT, phystime INT, ctime INT"
        + ", PRIMARY KEY (bpdatasetid, enginestep))"
    )
    Update(args, cur)


def MergeDBFiles(dbfiles: list):
    # read db files here
    result = list()
    for f1 in dbfiles:
        try:
            con = sqlite3.connect(f1)
        except sqlite3.Error as e:
            print(e)

        cur = con.cursor()
        try:
            cur.execute("select  * from bpdataset")
        except sqlite3.Error as e:
            print(e)
        record = cur.fetchall()
        for item in record:
            result.append({"name": item[0]})
        cur.close()
    return result


def List():
    if args.campaign_store is None:
        print("ERROR: Set --campaign_store for this command")
        return 1
    else:
        # List the local campaign store
        acaList = glob.glob(args.campaign_store + "/*.aca", recursive=True)
        if len(acaList) == 0:
            print("There are no campaign archives in  " + args.campaign_store)
            return 2
        else:
            startCharPos = len(args.campaign_store) + 1
            for f in acaList:
                print(f[startCharPos:])
    return 0


if __name__ == "__main__":
    args = SetupArgs()
    CheckCampaignStore(args)

    if args.command == "list":
        exit(List())

    if args.command == "delete":
        if exists(args.CampaignFileName):
            print(f"Delete archive {args.CampaignFileName}")
            remove(args.CampaignFileName)
            exit(0)
        else:
            print(f"ERROR: archive {args.CampaignFileName} does not exist")
            exit(1)

    if args.command == "create":
        print("Create archive")
        if exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} already exist")
            exit(1)
    elif args.command == "update" or args.command == "info":
        print(f"{args.command} archive")
        if not exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} does not exist")
            exit(1)

    con = sqlite3.connect(args.CampaignFileName)
    cur = con.cursor()

    if args.command == "info":
        Info(args, cur)
    else:
        CheckLocalCampaignDir(args)
        # List the local campaign directory
        dbFileList = glob.glob(args.LocalCampaignDir + "/*.acr")
        if len(dbFileList) == 0:
            print("There are no campaign data files in  " + args.LocalCampaignDir)
            exit(2)

        if args.command == "create":
            Create(args, cur)
        elif args.command == "update":
            Update(args, cur)

    cur.close()
    con.close()
