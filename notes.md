
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

