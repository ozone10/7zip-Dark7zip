!IFDEF WIN_CTRL_OBJS
DARK_MODE_OBJS = \
  $O\DarkMode.obj \
  $O\DarkModeHook.obj \
  $O\DarkModeSubclass.obj \
!ENDIF

OBJS = \
  $(OBJS) \
  $(DARK_MODE_OBJS) \
