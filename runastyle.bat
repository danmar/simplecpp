@REM Script to run AStyle on the sources

@SET STYLE=--style=stroustrup --indent=spaces=4 --indent-namespaces --lineend=linux --min-conditional-indent=0
@SET OPTIONS=--pad-header --unpad-paren --suffix=none --convert-tabs --attach-inlines

astyle %STYLE% %OPTIONS% *.h
astyle %STYLE% %OPTIONS% *.cpp
