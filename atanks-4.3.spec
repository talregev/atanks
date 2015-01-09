%define    name    atanks
%define    version 4.3
%define    release 1
%define    prefix  /usr

Summary:   A fun tank game, which plays like Scorched Earth.
Name:      %{name}
Version:   %{version}
Release:   %{release}
License:   GPL
Group:     Games Arcade
Vendor:    Jesse Smith <jessefrgsmith@yahoo.ca>
URL:       http://atanks.sourceforge.net/
Source0:   http://atanks.sourceforge.net/downloads/%{name}-%{version}.tar.gz
BuildArch: i386
#BuildRoot: /var/tmp/%{name}-root
Provides:  atanks

%description
Atomic Tanks is a simple tank game, similar to
Scorched Earth or Worms, where small tanks use large weapons
to destroy each other.

%prep
%setup

%build
make

%install
make install

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%{prefix}/games/bin/atanks
/usr/share/games/atanks/README_ru.txt
/usr/share/games/atanks/button/22.bmp
/usr/share/games/atanks/button/19.bmp
/usr/share/games/atanks/button/7.bmp
/usr/share/games/atanks/button/1.bmp
/usr/share/games/atanks/button/0.bmp
/usr/share/games/atanks/button/6.bmp
/usr/share/games/atanks/button/20.bmp
/usr/share/games/atanks/button/10.bmp
/usr/share/games/atanks/button/5.bmp
/usr/share/games/atanks/button/2.bmp
/usr/share/games/atanks/button/18.bmp
/usr/share/games/atanks/button/23.bmp
/usr/share/games/atanks/button/12.bmp
/usr/share/games/atanks/button/13.bmp
/usr/share/games/atanks/button/4.bmp
/usr/share/games/atanks/button/17.bmp
/usr/share/games/atanks/button/9.bmp
/usr/share/games/atanks/button/11.bmp
/usr/share/games/atanks/button/14.bmp
/usr/share/games/atanks/button/8.bmp
/usr/share/games/atanks/button/16.bmp
/usr/share/games/atanks/button/15.bmp
/usr/share/games/atanks/button/3.bmp
/usr/share/games/atanks/button/21.bmp
/usr/share/games/atanks/button/24.bmp
/usr/share/games/atanks/button/25.bmp
/usr/share/games/atanks/button/26.bmp
/usr/share/games/atanks/button/27.bmp
/usr/share/games/atanks/sound/0.wav
/usr/share/games/atanks/sound/2.wav
/usr/share/games/atanks/sound/3.wav
/usr/share/games/atanks/sound/1.wav
/usr/share/games/atanks/sound/11.wav
/usr/share/games/atanks/sound/12.wav
/usr/share/games/atanks/sound/10.wav
/usr/share/games/atanks/sound/8.wav
/usr/share/games/atanks/sound/5.wav
/usr/share/games/atanks/sound/9.wav
/usr/share/games/atanks/sound/6.wav
/usr/share/games/atanks/sound/7.wav
/usr/share/games/atanks/sound/4.wav
/usr/share/games/atanks/atanks.ico
/usr/share/games/atanks/missile/22.bmp
/usr/share/games/atanks/missile/19.bmp
/usr/share/games/atanks/missile/7.bmp
/usr/share/games/atanks/missile/1.bmp
/usr/share/games/atanks/missile/0.bmp
/usr/share/games/atanks/missile/6.bmp
/usr/share/games/atanks/missile/20.bmp
/usr/share/games/atanks/missile/25.bmp
/usr/share/games/atanks/missile/10.bmp
/usr/share/games/atanks/missile/5.bmp
/usr/share/games/atanks/missile/2.bmp
/usr/share/games/atanks/missile/18.bmp
/usr/share/games/atanks/missile/27.bmp
/usr/share/games/atanks/missile/23.bmp
/usr/share/games/atanks/missile/24.bmp
/usr/share/games/atanks/missile/12.bmp
/usr/share/games/atanks/missile/13.bmp
/usr/share/games/atanks/missile/28.bmp
/usr/share/games/atanks/missile/4.bmp
/usr/share/games/atanks/missile/17.bmp
/usr/share/games/atanks/missile/26.bmp
/usr/share/games/atanks/missile/9.bmp
/usr/share/games/atanks/missile/11.bmp
/usr/share/games/atanks/missile/14.bmp
/usr/share/games/atanks/missile/8.bmp
/usr/share/games/atanks/missile/16.bmp
/usr/share/games/atanks/missile/30.bmp
/usr/share/games/atanks/missile/15.bmp
/usr/share/games/atanks/missile/3.bmp
/usr/share/games/atanks/missile/29.bmp
/usr/share/games/atanks/missile/21.bmp
/usr/share/games/atanks/Changelog
/usr/share/games/atanks/atanks.png
/usr/share/games/atanks/title/1.bmp
/usr/share/games/atanks/title/0.bmp
/usr/share/games/atanks/title/2.bmp
/usr/share/games/atanks/title/4.bmp
/usr/share/games/atanks/title/3.bmp
/usr/share/games/atanks/unicode.dat
/usr/share/games/atanks/text/weapons_de.txt
/usr/share/games/atanks/text/retaliation_fr.txt
/usr/share/games/atanks/text/weapons_fr.txt
/usr/share/games/atanks/text/Help.txt
/usr/share/games/atanks/text/ingame.pt_BR.txt
/usr/share/games/atanks/text/war_quotes_ru.txt
/usr/share/games/atanks/text/gloat.txt
/usr/share/games/atanks/text/weapons.txt
/usr/share/games/atanks/text/retaliation.txt
/usr/share/games/atanks/text/ingame_sk.txt
/usr/share/games/atanks/text/instr_de.txt
/usr/share/games/atanks/text/kamikaze.pt_BR.txt
/usr/share/games/atanks/text/weapons.pt_BR.txt
/usr/share/games/atanks/text/war_quotes.txt
/usr/share/games/atanks/text/Help.pt_BR.txt
/usr/share/games/atanks/text/Help_fr.txt
/usr/share/games/atanks/text/suicide_fr.txt
/usr/share/games/atanks/text/weapons_ru.txt
/usr/share/games/atanks/text/gloat_ru.txt
/usr/share/games/atanks/text/gloat_sk.txt
/usr/share/games/atanks/text/ingame_de.txt
/usr/share/games/atanks/text/revenge_de.txt
/usr/share/games/atanks/text/ingame_ru.txt
/usr/share/games/atanks/text/retaliation.pt_BR.txt
/usr/share/games/atanks/text/revenge_sk.txt
/usr/share/games/atanks/text/suicide.txt
/usr/share/games/atanks/text/kamikaze_ru.txt
/usr/share/games/atanks/text/retaliation_ru.txt
/usr/share/games/atanks/text/instr_sk.txt
/usr/share/games/atanks/text/suicide.pt_BR.txt
/usr/share/games/atanks/text/instr.pt_BR.txt
/usr/share/games/atanks/text/kamikaze.txt
/usr/share/games/atanks/text/gloat_fr.txt
/usr/share/games/atanks/text/revenge.pt_BR.txt
/usr/share/games/atanks/text/instr_ru.txt
/usr/share/games/atanks/text/instr_fr.txt
/usr/share/games/atanks/text/revenge_fr.txt
/usr/share/games/atanks/text/revenge.txt
/usr/share/games/atanks/text/retaliation_de.txt
/usr/share/games/atanks/text/retaliation_sk.txt
/usr/share/games/atanks/text/kamikaze_fr.txt
/usr/share/games/atanks/text/suicide_sk.txt
/usr/share/games/atanks/text/suicide_ru.txt
/usr/share/games/atanks/text/revenge_ru.txt
/usr/share/games/atanks/text/weapons_sk.txt
/usr/share/games/atanks/text/kamikaze_sk.txt
/usr/share/games/atanks/text/kamikaze_de.txt
/usr/share/games/atanks/text/instr.txt
/usr/share/games/atanks/text/gloat_de.txt
/usr/share/games/atanks/text/ingame_fr.txt
/usr/share/games/atanks/text/gloat.pt_BR.txt
/usr/share/games/atanks/text/suicide_de.txt
/usr/share/games/atanks/text/Help_ru.txt
/usr/share/games/atanks/text/ingame.txt
/usr/share/games/atanks/tank/7.bmp
/usr/share/games/atanks/tank/1.bmp
/usr/share/games/atanks/tank/0.bmp
/usr/share/games/atanks/tank/6.bmp
/usr/share/games/atanks/tank/10.bmp
/usr/share/games/atanks/tank/5.bmp
/usr/share/games/atanks/tank/2.bmp
/usr/share/games/atanks/tank/4.bmp
/usr/share/games/atanks/tank/9.bmp
/usr/share/games/atanks/tank/8.bmp
/usr/share/games/atanks/tank/11.bmp
/usr/share/games/atanks/tank/12.bmp
/usr/share/games/atanks/tank/13.bmp
/usr/share/games/atanks/tank/14.bmp
/usr/share/games/atanks/tank/3.bmp
/usr/share/games/atanks/atanks.desktop
/usr/share/games/atanks/COPYING
/usr/share/games/atanks/Makefile
/usr/share/games/atanks/TODO
/usr/share/games/atanks/misc/7.bmp
/usr/share/games/atanks/misc/1.bmp
/usr/share/games/atanks/misc/0.bmp
/usr/share/games/atanks/misc/6.bmp
/usr/share/games/atanks/misc/10.bmp
/usr/share/games/atanks/misc/5.bmp
/usr/share/games/atanks/misc/2.bmp
/usr/share/games/atanks/misc/12.bmp
/usr/share/games/atanks/misc/13.bmp
/usr/share/games/atanks/misc/4.bmp
/usr/share/games/atanks/misc/9.bmp
/usr/share/games/atanks/misc/11.bmp
/usr/share/games/atanks/misc/14.bmp
/usr/share/games/atanks/misc/8.bmp
/usr/share/games/atanks/misc/16.bmp
/usr/share/games/atanks/misc/15.bmp
/usr/share/games/atanks/misc/3.bmp
/usr/share/games/atanks/README
/usr/share/games/atanks/stock/22.bmp
/usr/share/games/atanks/stock/19.bmp
/usr/share/games/atanks/stock/35.bmp
/usr/share/games/atanks/stock/7.bmp
/usr/share/games/atanks/stock/31.bmp
/usr/share/games/atanks/stock/1.bmp
/usr/share/games/atanks/stock/0.bmp
/usr/share/games/atanks/stock/69.bmp
/usr/share/games/atanks/stock/49.bmp
/usr/share/games/atanks/stock/56.bmp
/usr/share/games/atanks/stock/6.bmp
/usr/share/games/atanks/stock/48.bmp
/usr/share/games/atanks/stock/53.bmp
/usr/share/games/atanks/stock/55.bmp
/usr/share/games/atanks/stock/51.bmp
/usr/share/games/atanks/stock/20.bmp
/usr/share/games/atanks/stock/65.bmp
/usr/share/games/atanks/stock/25.bmp
/usr/share/games/atanks/stock/43.bmp
/usr/share/games/atanks/stock/78.bmp
/usr/share/games/atanks/stock/34.bmp
/usr/share/games/atanks/stock/57.bmp
/usr/share/games/atanks/stock/10.bmp
/usr/share/games/atanks/stock/33.bmp
/usr/share/games/atanks/stock/71.bmp
/usr/share/games/atanks/stock/45.bmp
/usr/share/games/atanks/stock/5.bmp
/usr/share/games/atanks/stock/2.bmp
/usr/share/games/atanks/stock/74.bmp
/usr/share/games/atanks/stock/18.bmp
/usr/share/games/atanks/stock/32.bmp
/usr/share/games/atanks/stock/27.bmp
/usr/share/games/atanks/stock/23.bmp
/usr/share/games/atanks/stock/24.bmp
/usr/share/games/atanks/stock/12.bmp
/usr/share/games/atanks/stock/76.bmp
/usr/share/games/atanks/stock/39.bmp
/usr/share/games/atanks/stock/13.bmp
/usr/share/games/atanks/stock/73.bmp
/usr/share/games/atanks/stock/28.bmp
/usr/share/games/atanks/stock/4.bmp
/usr/share/games/atanks/stock/58.bmp
/usr/share/games/atanks/stock/36.bmp
/usr/share/games/atanks/stock/17.bmp
/usr/share/games/atanks/stock/26.bmp
/usr/share/games/atanks/stock/42.bmp
/usr/share/games/atanks/stock/40.bmp
/usr/share/games/atanks/stock/68.bmp
/usr/share/games/atanks/stock/59.bmp
/usr/share/games/atanks/stock/46.bmp
/usr/share/games/atanks/stock/9.bmp
/usr/share/games/atanks/stock/54.bmp
/usr/share/games/atanks/stock/11.bmp
/usr/share/games/atanks/stock/14.bmp
/usr/share/games/atanks/stock/44.bmp
/usr/share/games/atanks/stock/8.bmp
/usr/share/games/atanks/stock/41.bmp
/usr/share/games/atanks/stock/16.bmp
/usr/share/games/atanks/stock/30.bmp
/usr/share/games/atanks/stock/61.bmp
/usr/share/games/atanks/stock/75.bmp
/usr/share/games/atanks/stock/15.bmp
/usr/share/games/atanks/stock/64.bmp
/usr/share/games/atanks/stock/63.bmp
/usr/share/games/atanks/stock/66.bmp
/usr/share/games/atanks/stock/47.bmp
/usr/share/games/atanks/stock/37.bmp
/usr/share/games/atanks/stock/3.bmp
/usr/share/games/atanks/stock/70.bmp
/usr/share/games/atanks/stock/29.bmp
/usr/share/games/atanks/stock/21.bmp
/usr/share/games/atanks/stock/62.bmp
/usr/share/games/atanks/stock/38.bmp
/usr/share/games/atanks/stock/77.bmp
/usr/share/games/atanks/stock/52.bmp
/usr/share/games/atanks/stock/60.bmp
/usr/share/games/atanks/stock/72.bmp
/usr/share/games/atanks/stock/50.bmp
/usr/share/games/atanks/stock/67.bmp
/usr/share/games/atanks/credits.txt
/usr/share/games/atanks/tankgun/1.bmp
/usr/share/games/atanks/tankgun/0.bmp
/usr/share/games/atanks/tankgun/2.bmp
/usr/share/games/atanks/tankgun/3.bmp
/usr/share/games/atanks/tankgun/4.bmp
/usr/share/games/atanks/tankgun/5.bmp
/usr/share/games/atanks/tankgun/6.bmp
/usr/share/games/atanks/tankgun/7.bmp

%changelog

