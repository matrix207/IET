##
## Global Package Definitions
##

## IET Release
%define iet_version 1.4.20

## Package Revision
%define revision 1

## Build Options (modify to tune your build)
# Build DKMS kernel module
#
# Two passes through rpmbuild are required, once for the userland
# binary package and another time with --target=noarch for the
# kernel dkms package.
%define dkms 0

# Build weak modules (KABI tracking) if supported
#
# If it isn't supported it will figure that out below
%define weak 1

# Build from SVN repository
%define svn 0
%define svn_rev HEAD
%define svn_url https://iscsitarget.svn.sourceforge.net/svnroot/iscsitarget/trunk

## Platform Definitions (you shouldn't need to modify these)
# Determine distribution
%define is_suse		%(test -e /etc/SuSE-release && echo 1 || echo 0)
%define is_fedora	%(test -e /etc/fedora-release && echo 1 || echo 0)
%define is_redhat	%(test -e /etc/redhat-release && echo 1 || echo 0)
%define is_mandrake	%(test -e /etc/mandrake-release && echo 1 || echo 0)
%define is_mandriva	%(test -e /etc/mandriva-release && echo 1 || echo 0)

# Define kernel version information
%{!?kernel:	%define kernel %(uname -r)}

%define kver	%(echo %{kernel} | sed -e 's/default//' -e 's/pae//i' -e 's/xen//' -e 's/smp//' -e 's/bigmem//' -e 's/hugemem//' -e 's/enterprise//' -e 's/-$//')
%define krel	%(echo %{kver} | sed -e 's/-/_/g')
%define ktype	%(echo kernel-%{kernel} | sed -e 's/%{kver}//' -e 's/--/-/' -e 's/-$//')

%define moddir  /lib/modules/%{kernel}
%define module	%{moddir}/extra/iscsi/iscsi_trgt.ko

# Set location of tools
%{!?__chkconfig:	%define __chkconfig	/sbin/chkconfig}
%{!?__depmod:		%define __depmod	/sbin/depmod}
%{!?__dkms:		%define __dkms		/usr/sbin/dkms}
%{!?__find:		%define __find		/usr/bin/find}
%{!?__modprobe:		%define __modprobe	/sbin/modprobe}
%{!?__service:		%define __service	/sbin/service}
%{!?__svn:		%define __svn		/usr/bin/svn}
%if %is_suse
%{!?__weak_modules:	%define __weak_modules	/usr/lib/module-init-tools/weak-modules}
%else
%{!?__weak_modules:	%define __weak_modules	/sbin/weak-modules}
%endif

# Subversion build information
%if %svn
%define iet_version %(%{__svn} info -r %{svn_rev} --non-interactive %{svn_url} | awk '{if ($1 == "Revision:") {print "svn_r"$2}}')
%endif

# Build weak module (KABI Tracking) on platforms that support it
%if %weak
%define weak %(test -x %{__weak_modules} && echo 1 || echo 0)
%endif

# Basic regex filters for unwanted dependencies (used for weak modules)
%if %weak
%define pro_filter ""
%if %is_redhat
%define req_filter "\\(fsync_bdev\\|sync_page_range\\|vfs_write\\|get_io_context\\|copy_io_context\\|put_io_context\\)"
%else
%define req_filter ""
%endif
%endif

# Define build user
%define user	%(whoami)


##
## Userland Package
##

## Information
Summary: iSCSI Enterprise Target
Name: iscsitarget
Version: %{iet_version}
Release: %{?revision: %{revision}}%{!?revision: 1}
License: GPL
Group: System Environment/Daemons
URL: http://sourceforge.net/projects/iscsitarget/
Packager: IET development team <iscsitarget-devel@lists.sourceforge.net>

## Source files
%if !%svn
Source0: %{name}-%{version}.tar.gz
%endif

## Patches
#Patch0: %{name}-example.p

## Install Requirements
Requires: %{name}-kmod = %{version}

## Build Requirements
BuildRequires: kernel >= 2.6
BuildRequires: gcc, make, patch, binutils, /usr/bin/install, openssl-devel
%if %is_suse
BuildRequires: kernel-source = %{kver}
%else
BuildRequires: %{ktype}-devel = %{kver}
%endif
%if %svn
BuildRequires: subversion
%endif

## Build Definitions
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-%{user}

## Description
%description
iSCSI Enterprise Target


##
## Kernel Module Package
##
%if %dkms
%ifarch noarch
%package -n dkms-%{name}

## Information
Summary: iSCSI Enterprise Target kernel module
Group: System Environment/Kernel
Release: %{release}

## Install Requirements
Requires: dkms >= 2, gcc, make, patch, binutils
%if %is_suse
Requires: kernel-source
%else
Requires: %{ktype}-devel
%endif

## Install Provides
Provides: kernel-modules
Provides: %{name}-kmod = %{version}

## Install Conflicts
Conflicts: kmod-%{name}

## Description
%description -n dkms-%{name}
iSCSI Enterprise Target kernel module
%endif
%else
%package -n kmod-%{name}

## Information
Summary: iSCSI Enterprise Target kernel module
Group: System Environment/Kernel
Release: %{release}_%{krel}

## Install Requirements
%if %weak
Requires: %{ktype}
Requires(post): %{__weak_modules}
Requires(postun): %{__weak_modules}
%else
Requires: %{ktype} = %{kver}
%endif
Requires(post): %{__depmod}
Requires(postun): %{__depmod}

## Install Provides
%if !%weak
Provides: kernel-modules = %{kver}
%endif
Provides: %{name}-kmod = %{version}

## Install Conflicts
Conflicts: dkms-%{name}

## Description
%description -n kmod-%{name}
iSCSI Enterprise Target kernel module
%endif


##
## Package Creation
##

## Preparation
%prep


## Setup
%if %svn
%setup -q -D -T -c -n %{name}-%{version}
if [ ! -f include/iet_u.h ]; then
%{__svn} export --force -r %{svn_rev} --non-interactive -q %{svn_url} .
%{__sed} -i -e "s/\(#define IET_VERSION_STRING\).*/\1\t\"%{version}\"/" include/iet_u.h
# Patches to apply to SVN
#%patch0 -p0
fi
%else
%setup -q -n %{name}-%{version}
# Patches to apply to release
#%patch0 -p0
%endif


## Build
%build
%{__make} distclean
%ifnarch noarch
%if %dkms
%{__make} usr
%else
%{__make} KERNELSRC=/lib/modules/%{kernel}/build
%endif
%endif


## Installation
%install
%{__rm} -rf %{buildroot}

%ifnarch noarch
%if %dkms
%{__make} install-usr install-doc install-etc KERNELSRC=/lib/modules/%{kernel}/build DISTDIR=%{buildroot}
%else
%{__make} install-files KERNELSRC=/lib/modules/%{kernel}/build DISTDIR=%{buildroot}
%{__rm} -f %{buildroot}/lib/modules/%{kernel}/modules.*
%endif
%if %is_redhat || %is_fedora
%{__mkdir} -p %{buildroot}/etc/rc.d
%{__mv} %{buildroot}/etc/init.d %{buildroot}/etc/rc.d
%endif
%{__rm} -rf %{buildroot}/usr/share/doc/iscsitarget
%elseif %dkms
%{__mkdir} -p %{buildroot}/usr/src/%{name}-%{version}
%{__cp} -r COPYING dkms.conf include kernel patches %{buildroot}/usr/src/%{name}-%{version}
%{__sed} -i -e "s/\(PACKAGE_VERSION=\).*/\1\"%{version}\"/" %{buildroot}/usr/src/%{name}-%{version}/dkms.conf
%endif

# Ugly hack to filter out unwanted dependencies
%if %weak
%global _use_internal_dependency_generator 0
%if %(test -n "%{pro_filter}" && echo 1 || echo 0)
%define iet_provides %{_tmppath}/iet_provides-%{user}
%{__cat} << EOF > %{iet_provides}
%{__find_provides} "\$@" | %{__grep} -v %{pro_filter}
exit 0
EOF
%{__chmod} 755 %{iet_provides}
%define __find_provides %{iet_provides}
%endif
%if %(test -n "%{req_filter}" && echo 1 || echo 0)
%define iet_requires %{_tmppath}/iet_requires-%{user}
%{__cat} << EOF > %{iet_requires}
%{__find_requires} "\$@" | %{__grep} -v %{req_filter}
exit 0
EOF
%{__chmod} 755 %{iet_requires}
%define __find_requires %{iet_requires}
%endif
%endif

## Cleaning
%clean
%{__rm} -rf %{buildroot}
%if %{?iet_provides:1}%{!?iet_provides:0}
%{__rm} -f %{iet_provides}
%endif
%if %{?iet_requires:1}%{!?iet_requires:0}
%{__rm} -f %{iet_requires}
%endif


## Post-Install Script
%ifnarch noarch
%post
%if %is_suse
%{__ln} -s %{_initrddir}/iscsi-target %{_sbindir}/rciscsi-target
%endif
%{__chkconfig} --add iscsi-target
%endif


## Pre-Uninstall Script
%ifnarch noarch
%preun
if [ "$1" = 0 ]; then
    %{__service} iscsi-target stop &>/dev/null
    %{__chkconfig} --del iscsi-target &>/dev/null
fi
%if %is_suse
%{__rm} -f %{_sbindir}/rciscsi-target
%endif
%endif


## Post-Uninstall Script
%ifnarch noarch
%postun
if [ "$1" != 0 ]; then
    %{__service} iscsi-target condrestart &>/dev/null
fi
%endif


## Post-Install Script (Kernel Module)
%if %dkms
%ifarch noarch
%post -n dkms-%{name}
%{__dkms} add -m %{name} -v %{version} --rpm_safe_upgrade
%{__dkms} build -m %{name} -v %{version}
%{__dkms} install -m %{name} -v %{version} --force
%endif
%else
%post -n kmod-%{name}
%{__depmod} %{kernel} -a
%if %weak
if [ -x %{__weak_modules} ]; then
    echo %{module} | %{__weak_modules} --add-modules
fi
%endif
%endif


## Pre-Uninstall Script (Kernel Module)
%if %dkms
%ifarch noarch
%preun -n dkms-%{name}
%{__dkms} remove -m %{name} -v %{version} --all --rpm_safe_upgrade
%endif
%else
%preun -n kmod-%{name}
%{__modprobe} -r -q --set-version %{kernel} iscsi_trgt
%if %weak
if [ -x %{__weak_modules} ]; then
    echo %{module} | %{__weak_modules} --remove-modules
fi
%endif
%endif


## Post-Uninstall Script (Kernel Module)
%if !%dkms
%postun -n kmod-%{name}
%{__depmod} %{kernel} -a
%endif


## File Catalog
%ifnarch noarch
%files
%defattr(-, root, root)
%{_sbindir}/*
%{_mandir}/man?/*
%{_initrddir}/*
%config(noreplace) %{_sysconfdir}/iet/ietd.conf
%config(noreplace) %{_sysconfdir}/iet/initiators.allow
%config(noreplace) %{_sysconfdir}/iet/targets.allow
%doc ChangeLog COPYING RELEASE_NOTES README README.vmware README.initiators
%endif


## File Catalog (Kernel Module)
%if %dkms
%ifarch noarch
%files -n dkms-%{name}
%defattr(-, root, root)
/usr/src/%{name}-%{version}/*
%endif
%else
%files -n kmod-%{name}
%defattr(-, root, root)
/lib/modules/%{kernel}
%endif


%changelog
* Tue Mar 30 2010 Ross Walker <rswwalker at gmail dot com> - 1.4.20
- Changed DKMS module name to dkms-iscsitarget from kmod-iscsitarget
- Added Provides: kernel-modules to DKMS module so multiple versions can
  be managed by DKMS at once.
- Added conflicts to dkms and kmod for each other so only one type can
  be installed at a time.
- Added --rpm_safe_upgrade to DKMS add/remove for RPM-DKMS sync
- Added --force to DKMS install in case version changed, but code didn't

* Mon Mar 29 2010 Ross Walker <rswwalker at gmail dot com> - 1.4.20
- Updated SVN build defines.
- Added weak build define to manually choose KABI tracking or per-kernel
- Fixed issue with upgrading KABI tracking module by removing
  Provides: kernel-modules to weak builds, kept for per-kernel builds
  if you install weak built module, all per-kernels are uninstalled,
  you cannot install a per-kernel while a weak built is installed.

* Mon Mar 08 2010 Ross Walker <rswwalker at gmail dot com> - 1.4.19
- removed openssl-devel build requirement

* Wed Oct 14 2009 Ross Walker <rswwalker at gmail dot com> - 1.4.18
- Added more macros for better cross-distro support
- Modified Matt's update for better Redhat-SuSE compatibility

* Fri Oct 09 2009 Matthew Wild <matthew.wild at stfc dot ac dot uk> - 1.4.18
- Added openSuSE specific configuration
- Tidied up files section for init.d|rc.d
- run depmod with -a rather than -A option

* Wed Sep 25 2009 Ross Walker <rswwalker at gmail dot com> - 0.4.17-244
- SuSE puts weak-modules under /usr/lib/module-init-tools
- Kernel module now located in /lib/modules/<kernel>/extra/iscsi

* Wed Sep 25 2009 Ross Walker <rswwalker at gmail dot com> - 0.4.17-242
- Added ability to build weak modules for platforms that support them
- Cleaned up logic a little
- Made safe for multi-user builds
- Redacted old spec file maintainer's email address

* Wed Sep 22 2009 Ross Walker <rswwalker at gmail dot com> - 0.4.17-236
- Updated file catalog for new config directory

* Wed Sep 09 2009 Ross Walker <rswwalker at gmail dot com> - 0.4.17-226
- Added ability to build directly from subversion repo
- Added ability to build dkms kernel module

* Mon Nov 10 2008 Ross Walker <rswwalker at gmail dot com> - 0.4.17-177
- Changed kernel-module naming to kmod
- Updated versioning

* Fri Feb 16 2007 Ross Walker <rswwalker at gmail dot com> - 0.4.14-96
- Reworked spec file for latest release
- Commented and cleaned up sections
- Added additional documents to %files

* Mon Nov 21 2005 Bastiaan Bakker <redacted> - 0.4.13-0.1266.1
- upstream snapshot 1266
- added condrestart patch
- stop and start service on update or removal

* Sun Nov 13 2005 Bastiaan Bakker <redacted> - 0.4.13-0.1264.2
- run %post and %preun for kernel package, not main package

* Sun Nov 13 2005 Bastiaan Bakker <redacted> - 0.4.13-0.1264.1
- updated to snapshot 1264

* Thu Nov 03 2005 Bastiaan Bakker <redacted> - 0.4.12-6
- added openssl-devel build requirement
- removed '.ko' extension in modprobe command

* Wed Nov 02 2005 Bastiaan Bakker <redacted> - 0.4.12-5
- fixed kernel-devel BuildRequires

* Fri Sep 23 2005 Bastiaan Bakker <redacted> - 0.4.12-4
- fixed modprobe -r 'FATAL' message
- run depmod with correct kernel version

* Fri Sep 23 2005 Bastiaan Bakker <redacted> - 0.4.12-3
- added config files
- set kernel module file permissions to 744
- fixed provides/requires of kernel module
- removed BuildArch restriction

* Thu Sep 22 2005 Bastiaan Bakker <redacted> - 0.4.12-2
- create separate subpackage for kernel module
- include man pages
- added kernel compatibility patch for kernels < 2.6.11

* Wed Aug 03 2005 Bastiaan Bakker <redacted>
- First version.

