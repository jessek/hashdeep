HOST=""
if [ -d /usr/local/i386-mingw32-4.3.0 ] ; 
then
  export PATH=/usr/local/i386-mingw32-4.3.0/bin:$PATH
  HOST=i386-mingw32-4.3.0
fi

if [ -d /usr/i586-mingw32msvc ] ; 
then
  export PATH=/usr/i586-mingw32msvc/bin:$PATH
  HOST=i586-mingw32msvc
fi

if [ -d /usr/i686-w64-mingw32 ] ;
then 
  export PATH=/usr/local/i686-w64-mingw32:$PATH
  HOST=i686-w64-mingw32
fi

if [ x$HOST = x"" ] ; 
then 
  echo Cannot configure for mingw
  exit 1
fi

./configure --host=$HOST
echo Configured for $HOST

