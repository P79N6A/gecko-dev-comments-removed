




























#include "def.h"

#ifdef _MSC_VER
#include <windows.h>
static int
does_file_exist(char *file)
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  BOOL b = GetFileAttributesExA(file, GetFileExInfoStandard, &data);
  if (!b)
    return 0;
  return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
#else
static int
does_file_exist(char *file)
{
  struct stat sb;
  return stat(file, &sb) == 0 && !S_ISDIR(sb.st_mode);
}
#endif

extern struct	inclist	inclist[ MAXFILES ],
			*inclistp, *inclistnext;
extern char	*includedirs[ ],
		**includedirsnext;
extern char	*notdotdot[ ];
extern boolean show_where_not;
extern boolean warn_multiple;

static boolean
isdot(char *p)
{
	if(p && *p++ == '.' && *p++ == '\0')
		return(TRUE);
	return(FALSE);
}

static boolean
isdotdot(char *p)
{
	if(p && *p++ == '.' && *p++ == '.' && *p++ == '\0')
		return(TRUE);
	return(FALSE);
}

static boolean
issymbolic(char *dir, char *component)
{
#ifdef S_IFLNK
	struct stat	st;
	char	buf[ BUFSIZ ], **pp;

	sprintf(buf, "%s%s%s", dir, *dir ? "/" : "", component);
	for (pp=notdotdot; *pp; pp++)
		if (strcmp(*pp, buf) == 0)
			return (TRUE);
	if (lstat(buf, &st) == 0
	&& (st.st_mode & S_IFMT) == S_IFLNK) {
		*pp++ = copy(buf);
		if (pp >= &notdotdot[ MAXDIRS ])
			fatalerr("out of .. dirs, increase MAXDIRS\n");
		return(TRUE);
	}
#endif
	return(FALSE);
}






static void
remove_dotdot(char *path)
{
	register char	*end, *from, *to, **cp;
	char		*components[ MAXFILES ],
			newpath[ BUFSIZ ];
	boolean		component_copied;

	


	to = newpath;
	if (*path == '/')
		*to++ = '/';
	*to = '\0';
	cp = components;
	for (from=end=path; *end; end++)
		if (*end == '/') {
			while (*end == '/')
				*end++ = '\0';
			if (*from)
				*cp++ = from;
			from = end;
		}
	*cp++ = from;
	*cp = NULL;

	


	cp = components;
	while(*cp) {
		if (!isdot(*cp) && !isdotdot(*cp) && isdotdot(*(cp+1))
		    && !issymbolic(newpath, *cp))
		{
		    char **fp = cp + 2;
		    char **tp = cp;

		    do 
			*tp++ = *fp; 
		    while (*fp++);
		    if (cp != components)
			cp--;	
		} else {
		    cp++;
		}
	}
	


	cp = components;
	component_copied = FALSE;
	while(*cp) {
		if (component_copied)
			*to++ = '/';
		component_copied = TRUE;
		for (from = *cp; *from; )
			*to++ = *from++;
		*to = '\0';
		cp++;
	}
	*to++ = '\0';

	


	strcpy(path, newpath);
}




struct inclist *
newinclude(char *newfile, char *incstring)
{
	register struct inclist	*ip;

	


	ip = inclistp++;
	if (inclistp == inclist + MAXFILES - 1)
		fatalerr("out of space: increase MAXFILES\n");
	ip->i_file = copy(newfile);

	if (incstring == NULL)
		ip->i_incstring = ip->i_file;
	else
		ip->i_incstring = copy(incstring);

	inclistnext = inclistp;
	return(ip);
}

void
included_by(struct inclist *ip, struct inclist *newfile)
{
	register int i;

	if (ip == NULL)
		return;
	





	if (ip->i_list == NULL) {
		ip->i_list = (struct inclist **)
			malloc(sizeof(struct inclist *) * ++ip->i_listlen);
		ip->i_merged = (boolean *)
		    malloc(sizeof(boolean) * ip->i_listlen);
	} else {
		for (i=0; i<ip->i_listlen; i++)
			if (ip->i_list[ i ] == newfile) {
			    i = strlen(newfile->i_file);
			    if (!(ip->i_flags & INCLUDED_SYM) &&
				!(i > 2 &&
				  newfile->i_file[i-1] == 'c' &&
				  newfile->i_file[i-2] == '.'))
			    {
				
				
				
				if (warn_multiple)
				{
					warning("%s includes %s more than once!\n",
						ip->i_file, newfile->i_file);
					warning1("Already have\n");
					for (i=0; i<ip->i_listlen; i++)
						warning1("\t%s\n", ip->i_list[i]->i_file);
				}
			    }
			    return;
			}
		ip->i_list = (struct inclist **) realloc(ip->i_list,
			sizeof(struct inclist *) * ++ip->i_listlen);
		ip->i_merged = (boolean *)
		    realloc(ip->i_merged, sizeof(boolean) * ip->i_listlen);
	}
	ip->i_list[ ip->i_listlen-1 ] = newfile;
	ip->i_merged[ ip->i_listlen-1 ] = FALSE;
}

void
inc_clean (void)
{
	register struct inclist *ip;

	for (ip = inclist; ip < inclistp; ip++) {
		ip->i_flags &= ~MARKED;
	}
}

struct inclist *
inc_path(char *file, char *include, int type)
{
	static char		path[ BUFSIZ ];
	register char		**pp, *p;
	register struct inclist	*ip;

	



	if ((type == INCLUDE) || (type == INCLUDEDOT))
		inclistnext = inclist;
	ip = inclistnext;

	for (; ip->i_file; ip++) {
		if ((strcmp(ip->i_incstring, include) == 0) &&
		    !(ip->i_flags & INCLUDED_SYM)) {
			inclistnext = ip + 1;
			return ip;
		}
	}

	if (inclistnext == inclist) {
		



		if ((type == INCLUDEDOT) ||
		    (type == INCLUDENEXTDOT) ||
		    (*include == '/')) {
			if (does_file_exist(include))
				return newinclude(include, include);
			if (show_where_not)
				warning1("\tnot in %s\n", include);
		}

		



		if ((type == INCLUDEDOT) || (type == INCLUDENEXTDOT)) {
			for (p=file+strlen(file); p>file; p--)
				if (*p == '/')
					break;
			if (p == file) {
				strcpy(path, include);
			} else {
				strncpy(path, file, (p-file) + 1);
				path[ (p-file) + 1 ] = '\0';
				strcpy(path + (p-file) + 1, include);
			}
			remove_dotdot(path);
			if (does_file_exist(path))
				return newinclude(path, include);
			if (show_where_not)
				warning1("\tnot in %s\n", path);
		}
	}

	



	if ((type == INCLUDE) || (type == INCLUDEDOT))
		includedirsnext = includedirs;
	pp = includedirsnext;

	for (; *pp; pp++) {
		sprintf(path, "%s/%s", *pp, include);
		remove_dotdot(path);
		if (does_file_exist(path)) {
			includedirsnext = pp + 1;
			return newinclude(path, include);
		}
		if (show_where_not)
			warning1("\tnot in %s\n", path);
	}

	return NULL;
}
