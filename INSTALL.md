# Installing the EnnoDB FastCGI service

The following will install all of the FastCGI services in this
repository (prefix, ennodb, counter). If you only need to install of
them, YMMV and you need to change the install rule in the Makefile.

The services are installed into /opt. If you prefer another location,
like /usr/local, then you need to change the PREFIX variable in the
Makefile.

This procedure assumes that you have a Debian-based system, and
requires a working compiler and the libfcgi-dev package. If you are
unsure that these are installed, run:

	sudo apt-get install build-essential libfcgi-dev spawn-fcgi

## download and build the source

First, clone the git repository:

	git clone https://github.com/badgerman/ennodb.git
	cd ennodb
	git submodule update

Now, build and install the code:
	make
	make test
	make install

The install procedure will ask you for your sudo password, because it
copies files into locations that require administrative permission.

## Start the services, add them to nginx

The services are installed in /etc/init.d, so they are easy to start:
	
	sudo service ennodb start

Next, we need to tell nginx about the service. If you want ennodb to
be available as http://your.host.name/ennodb/ then there is a
configuration file (fastcgi.conf) for that, which was part of the
installation process above. In your /etc/nginx/sites-available/default
file, add one line inside the `server { [...] }` definition:
	
	include /opt/share/doc/ennodb/examples/nginx.conf;

Now just run `sudo service nginx reload` to activate your change, and
try it!

    # store a value for key "foo":
    curl --data "Hello World" http://localhost/ennodb/foo
    # retrieve the value for key "foo":
    curl http://localhost/ennodb/foo

If this prints "Hello World", then you've done it. Alternatively, open
http://your.host.name/keyval.html in your browser and fiddle around
with it.
