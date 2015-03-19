# Introduction #
This page contains instructions for checking out purple-groupme source code and compiling and installing the plugin.

1. Getting the Code - how to check out purple-groupme source from googleCode.

2. Building the Plugin - how to compile the purple-groupme plugin.

3. Installing the Plugin - how to make the purple-groupme plugin available for use in pidgin.


# Getting the Code #
## Get the pidgin dependencies ##
Install dependencies (it's easy!).
If you do not have it, run:
```
sudo apt-get build-dep pidgin
```


## Get the purple-groupme source code ##
Source code is available from google-code via svn.

```
i.e.:
cd ~

# Project members authenticate over HTTPS to allow committing changes.
svn checkout https://purple-groupme.googlecode.com/svn/trunk/ purple-groupme --username <username>

When prompted, enter your generated googlecode.com password. 

# Non-members may check out a read-only working copy anonymously over HTTP.
svn checkout http://purple-groupme.googlecode.com/svn/trunk/ purple-groupme
```

# Building the Code #
```
cd purple-groupme
make clean
make libgroupme.so
```

As a convenience, a slightly different command will make and "install" the plugin.
```
cd purple-groupme
make clean
make groupme
```


# Installing the Plugin #
In order for libpurple applications (i.e. pidgin) to find purple-groupme, it must be installed.

For a single user, this can be easily accomplished by copying the plugin binary to a special directory:

```
mkdir -p ~/.purple/plugins
cp libgroupme.so ~/.purple/plugins
```

Now restart pidgin, Manage Accounts -> Add Account and you should see "GroupMe" in the pull-down list of protocols.

Enjoy!