# dcs-autocomp
DLS Controls Group Autocomplete Scripts and Applications

Supported Commands
==================

The following commands in the DCS environment are supported with bash tabcompletion:

 * console
 * ioc-connect
 * caget
 * caput
 * camonitor

How to use
==========

Source the complete_console.sh once into your environment and enjoy bash tab-completion on the supported bash commands.

```shell script
source /path/to/complete_console.sh
```

Then for example use tab-complete to find a PV on I12:
```
caget BL12I-MO-<TAB-TAB>
BL12I-MO-ECR-01    BL12I-MO-GEO-24    BL12I-MO-IOC-07    BL12I-MO-STEP-03   BL12I-MO-STEP-14   BL12I-MO-TAB-07
BL12I-MO-ECR-02    BL12I-MO-GEO-25    BL12I-MO-IOC-08    BL12I-MO-STEP-04   BL12I-MO-STEP-15   BL12I-MO-TAB-08
BL12I-MO-FANC-06   BL12I-MO-GEO-26    BL12I-MO-IOC-09    BL12I-MO-STEP-05   BL12I-MO-STEP-16   BL12I-MO-TAB-09
BL12I-MO-FANC-07   BL12I-MO-GONIO-01  BL12I-MO-IOC-10    BL12I-MO-STEP-06   BL12I-MO-STEP-27   BL12I-MO-TABXZ-01
BL12I-MO-GEO-12    BL12I-MO-IOC-01    BL12I-MO-IOC-11    BL12I-MO-STEP-08   BL12I-MO-TAB-01    BL12I-MO-USER-01
BL12I-MO-GEO-16    BL12I-MO-IOC-02    BL12I-MO-IOC-12    BL12I-MO-STEP-09   BL12I-MO-TAB-02    BL12I-MO-USER-02
BL12I-MO-GEO-19    BL12I-MO-IOC-03    BL12I-MO-IOC-13    BL12I-MO-STEP-10   BL12I-MO-TAB-03    BL12I-MO-USER-03
BL12I-MO-GEO-21    BL12I-MO-IOC-04    BL12I-MO-IOC-14    BL12I-MO-STEP-11   BL12I-MO-TAB-04    BL12I-MO-USER2-01
BL12I-MO-GEO-22    BL12I-MO-IOC-05    BL12I-MO-LNMOT-01  BL12I-MO-STEP-12   BL12I-MO-TAB-05    BL12I-MO-XPS3-01
BL12I-MO-GEO-23    BL12I-MO-IOC-06    BL12I-MO-PMAC-09   BL12I-MO-STEP-13   BL12I-MO-TAB-06    
caget BL12I-MO-GO<TAB>
caget BL12I-MO-GONIO-01:<TAB-TAB>
BL12I-MO-GONIO-01:DEVSTA      BL12I-MO-GONIO-01:INFO:NCURR  BL12I-MO-GONIO-01:INFO:NTEMP  BL12I-MO-GONIO-01:NAME
BL12I-MO-GONIO-01:INFO:MTYPE  BL12I-MO-GONIO-01:INFO:NFLOW  BL12I-MO-GONIO-01:MOTORSTA    
caget BL12I-MO-GONIO-01:MO<TAB>
caget BL12I-MO-GONIO-01:MOTORSTA <enter>
BL12I-MO-GONIO-01:MOTORSTA     0

```

Can't remember the syntax for the caget -d flag? Use tab-complete there as well:

```shell script
caget -d <TAB>
caget -d DBR_<TAB-TAB>
DBR_CHAR           DBR_CTRL_LONG      DBR_GR_DOUBLE      DBR_INT            DBR_STS_ENUM       DBR_TIME_DOUBLE
DBR_CLASS_NAME     DBR_CTRL_SHORT     DBR_GR_ENUM        DBR_LONG           DBR_STS_FLOAT      DBR_TIME_ENUM
DBR_CTRL_CHAR      DBR_CTRL_STRING    DBR_GR_FLOAT       DBR_SHORT          DBR_STS_INT        DBR_TIME_FLOAT
DBR_CTRL_DOUBLE    DBR_DOUBLE         DBR_GR_INT         DBR_STRING         DBR_STS_LONG       DBR_TIME_INT
DBR_CTRL_ENUM      DBR_ENUM           DBR_GR_LONG        DBR_STSACK_STRING  DBR_STS_SHORT      DBR_TIME_LONG
DBR_CTRL_FLOAT     DBR_FLOAT          DBR_GR_SHORT       DBR_STS_CHAR       DBR_STS_STRING     DBR_TIME_SHORT
DBR_CTRL_INT       DBR_GR_CHAR        DBR_GR_STRING      DBR_STS_DOUBLE     DBR_TIME_CHAR      DBR_TIME_STRING
caget -d DBR_D<TAB>
caget -d DBR_DOUBLE <pvname> <enter>
```

Can't remember the obscure syntax for timestamps on camonitor -t? Tab-complete to the rescue:
```shell script
camonitor -t<TAB-TAB>
 's' = CA server (remote) timestamps
 'c' = CA client (local) timestamps (shown in '()'s)
 'n' = no timestamps
 'r' = relative timestamps (time elapsed since start of program)
 'i' = incremental timestamps (time elapsed since last update)
 'I' = incremental timestamps (time elapsed since last update, by channel)
sr si sI cr ci cI nr ni nI
camonitor -t s<TAB>
si  sI  sr  
camonitor -t sI <pvname> <enter>
```

DCS Tools Installation
======================

This module is installed as a Controls Group 'tool' named 'dcs-autocomp'. To use the latest default version in your 
environment, place the following command in your ~/.bash_profile:

```shell script
source /dls_sw/prod/tools/RHEL7-x86_64/defaults/bin/.dcs_complete_console.sh
```

Process Variable Autocompletion
===============================

EPICS process variables (PVs) can be autocompleted for CLI tools: caget, caput, camonitor. The PVs are identified by 
scanning through IOC databases for record names (autocomplete for sub-record fields is no supported - nice TODO though...).

Relevant IOCs to scan are identified through the redirector table and in order to achieve a snappy response, some 
optimisations are done. In case of difficulties (i.e. a certain PV name does not appear in the autocomplete list), please
keep the following in mind:

 * Only IOCs/versions defined in the redirector table (i.e. configure-ioc) are scanned for PV record names
 * Only IOCs in the redirector that match the first (domain) part of the PV name (i.e. BL12I- or FE23I) are actively
scanned for PV names
 * As an optimisation, caching is implemented per domain (for each user)
   * IOC/DBs are re-scanned if the redirector timestamp (last change) is newer than the cache file timestamp
   * Cache files with lists of records are stored in `/tmp/<user>-<domain>-rec`
 * The current working directory (cwd) is always scanned for records in `./db/*.db` files
   * This is to enable auto-complete on local development IOCs that have not yet made it to the redirector
   * Caching is **not** implemented for these local databases/records.
