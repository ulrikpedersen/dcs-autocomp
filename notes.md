
Run test commands
==================

Most basic: 
```
dcs_compgen --redirector=/dls_sw/prod/etc/redirector/redirect_table BL12I-
```

Time it to measure performance changes: 
```
time dcs_compgen --redirector=/dls_sw/prod/etc/redirector/redirect_table BL12I-
```

Reset caches and then re-run: 
```
rm /tmp/*-rec && time dcs_compgen --redirector=/dls_sw/prod/etc/redirector/redirect_table BL12I-
```

System Test
===========

Use the 'bats' application to run a suite of system tests of dcs_compgen.

Run the executable, piping into diff against the above file.

Check the diff return code: 0=same, 1=diff, 2=bang

