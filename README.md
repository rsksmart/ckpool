# CKPool with RSK Merged Mining Capabilities

This repository is based on [CKPool original](https://bitbucket.org/ckolivas/ckpool) repository  and has all the neccessary changes to allow the pool to do merged mining with RSK.
Said changes can be found in the master branch, which is periodically updated with changes from CKPool original.

If you need more information about CKPool origina, please refer to its [README](https://github.com/rsksmart/ckpool/blob/master/README_original.md)

If you are planning to use this code, please check that is up to date with the version of CKPool original that you are using.

## Merged Mining settings

The following settings must be configured on `ckpool.conf` to do merged mining with RSK.

```
"rskd" :  [
	{
		"url" : "localhost:4444",
		"auth" : "user",
		"pass" : "pass"
	}
]
```
`url` is the address and port where the RskJ node is listening

`"rskpollperiod": 2` indicates the frequency in seconds to poll RskJ node for work.

`"rsknotifypolicy": 2` indicates when to trigger updates to miners. 
- 0 is only when a new bitcoin work is received 
- 1 is when an rsk work is received  
- 2 is the same as 1 but sending `clean_jobs` parameter from stratum protocol in `true` 
