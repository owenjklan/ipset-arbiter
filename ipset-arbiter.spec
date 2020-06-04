Summary: ipset Arbiter
Name: ipset-arbiter
Version: 1.0
Release: 1%{?dist}
License: GPL
BuildArch: x86_64
Group: Networking Tools
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-%{release}-buildroot-%(whoami)

%description
Standalone daemon service that allows clients to manipulate ipsets.
Useful for environments that do a lot of ipset manipulation where
the only other alternative is "shelling out" to the ipset command.
Which isn't particularly efficient.

%prep
%setup -c

%build
cd src
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -m 755 -p $RPM_BUILD_ROOT/usr/sbin
install -m 755 src/ctmortician $RPM_BUILD_ROOT/usr/sbin/

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/sbin/ctmortician
