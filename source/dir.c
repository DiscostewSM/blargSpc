
#include "dir.h"

struct DIRLIST
{
	struct DIRLIST * pNext;
	void * item;
};

struct DIRLISTITEM
{
	char * name;
	int type;
};

struct DIRLIST * head = NULL;
struct DIRLIST * curr = NULL;
struct DIRLISTITEM ** entryIdx;

char * dirextlist = NULL;
char * dirfullpath = NULL;


int nEntries = 0;

bool dirInited = false;


static int _diritemcmp(void * first, void * second)
{
	struct DIRLISTITEM * firstli = (struct DIRLISTITEM *)(first);
	struct DIRLISTITEM * secondli = (struct DIRLISTITEM *)(second);
	if(firstli->type != secondli->type)
	{
		if(firstli->type > secondli->type)
			return -1;
		return 1;
	}
	return strncasecmp(firstli->name, secondli->name, 0x105);
}

struct DIRLIST * _dircreatelist(void * item)
{
	struct DIRLIST * ptr = (struct DIRLIST*)linearAlloc(sizeof(struct DIRLIST));
	if(ptr == NULL)
		return NULL;
	ptr->item = item;
	ptr->pNext = NULL;
	head = curr = ptr;
	return ptr;
}

struct DIRLIST * _diraddtolist(void * item)
{
	if(head == NULL)
		return _dircreatelist(item);
	struct DIRLIST * ptr = (struct DIRLIST*)linearAlloc(sizeof(struct DIRLIST));
	if(ptr == NULL)
		return NULL;
	ptr->item = item;
	ptr->pNext = NULL;

	curr->pNext = ptr;
	curr = ptr;

	return ptr;
}

void _dirdeleteitem(void * item)
{
	struct DIRLISTITEM * thisitem = (struct DIRLISTITEM *)(item);
	linearFree(thisitem->name);
	linearFree(item);
}

void _dirclearlist()
{
	if(dirfullpath)
	{
		linearFree(dirfullpath);
		dirfullpath = NULL;
	}

	if(head != NULL)
	{
		linearFree(entryIdx);
		while(head != NULL)
		{
			curr = head;
			_dirdeleteitem(curr->item);
			head = head->pNext;
			linearFree(curr);
		}
	}
}

bool _dirisgoodentry(struct dirent *entry)
{
	if(entry->d_type & DT_DIR) return true;

	char * name = (char *)entry->d_name;
	char * ext = strrchr(name, '.') + 1;
	if(strlen(ext) == 0) return false;
	
	char * pch = dirextlist;
	while(*pch != '\0')
	{
		size_t slen = strcspn(pch, "|");
		if(slen > 0)
			if(strncasecmp(ext, pch, slen) == 0)
				return true;
		pch += slen + 1;
	}
	
	return false;
}

struct DIRLIST * _dirsortlist(struct DIRLIST * pList) {
    // zero or one element in list
    if(pList == NULL || pList->pNext == NULL)
        return pList;
    // head is the first element of resulting sorted list
    struct DIRLIST * head = NULL;
    while(pList != NULL) {
        struct DIRLIST * current = pList;
        pList = pList->pNext;
        if(head == NULL || (_diritemcmp(current->item, head->item) < 0)) {
            // insert into the head of the sorted list
            // or as the first element into an empty sorted list
            current->pNext = head;
            head = current;
        } else {
            // insert current element into proper position in non-empty sorted list
            struct DIRLIST * p = head;
            while(p != NULL) {
                if(p->pNext == NULL || // last element of the sorted list
					(_diritemcmp(current->item, p->pNext->item) < 0))
                {
                    // insert into middle of the sorted list or as the last element
                    current->pNext = p->pNext;
                    p->pNext = current;
                    break; // done
                }
                p = p->pNext;
            }
        }
    }
    return head;
}

char * _dirallocstring(char * string)
{
	size_t slen = strlen(string);
	if(slen == 0) return NULL;

	char * copy = (char *)linearAlloc(slen + 1);
	strncpy(copy, string, slen + 1);
	return copy;
}

char * dirGetCurrentDir(bool alloc)
{
	if(!dirInited)
		return NULL;

	if(alloc)
		return _dirallocstring(dirfullpath);

	return dirfullpath;
}

char * dirGetEntry(int idx, bool alloc)
{
	if((!dirInited) || (idx >= nEntries))
		return NULL;

	if(alloc)
		return _dirallocstring(entryIdx[idx]->name);

	return (entryIdx[idx]->name);
}


bool dirIsEntryDir(int idx)
{
	if((!dirInited) || (idx >= nEntries))
		return false;

	return (entryIdx[idx]->type > 0);
}

bool dirIsEntryFile(int idx)
{
	if((!dirInited) || (idx >= nEntries))
		return false;
	
	return (entryIdx[idx]->type == 0);
}

int dirGetEntryCount()
{
	return nEntries;
}

bool dirReadDir(char * dirPath)
{
	if((!dirInited) || (strlen(dirPath) == 0))
		return false;

	int i;
	struct dirent *entry;
	size_t slen;

	DIR * pDir = opendir(dirPath);

	if(pDir == NULL)
		return false;

	_dirclearlist();

	head = NULL;
	curr = NULL;

	slen = strlen(dirPath);
	dirfullpath = (char *)linearAlloc(slen + 1);
	strncpy(dirfullpath, dirPath, slen);
	dirfullpath[slen] = '\0';

	if(strcmp(dirPath, "/") == 0)
		nEntries = 0;
	else
	{
		struct DIRLISTITEM * newItem = (struct DIRLISTITEM *)linearAlloc(sizeof(struct DIRLISTITEM));
		newItem->name = (char*)linearAlloc(0x4);
		strncpy(newItem->name, "/..\0", 0x4);
		newItem->type = 2;
		_diraddtolist((void *)(newItem));
		nEntries = 1;
	}

	for(;;)
	{
		entry = readdir(pDir);
		if(entry == NULL) break;
		if(dirextlist != NULL && !_dirisgoodentry(entry)) continue;
		slen = strlen(entry->d_name);
		struct DIRLISTITEM * newItem = (struct DIRLISTITEM *)linearAlloc(sizeof(struct DIRLISTITEM *));
		newItem->name = (char *)linearAlloc(slen + 2);
		newItem->type = (entry->d_type & DT_DIR ? 1 : 0);

		if(newItem->type)
			newItem->name[0] = '/';
		strncpy(&(newItem->name[newItem->type]), entry->d_name, slen);
		newItem->name[slen + newItem->type] = '\0';
		
		_diraddtolist((void *)(newItem));
		nEntries++;
	}

	closedir(pDir);

	head = _dirsortlist(head);

	entryIdx = (struct DIRLISTITEM **)linearAlloc(nEntries * sizeof(struct DIRLISTITEM *));

	curr = head;
	for(i = 0; i < nEntries; i++)
	{
		entryIdx[i] = (struct DIRLISTITEM *)(curr->item);
		curr = curr->pNext;
	}

	return true;
}

bool dirEnterDirectory(int idx)
{
	if(!dirIsEntryDir(idx))
		return false;

	char * newdir = dirGetCurrentDir(true);

	if(entryIdx[idx]->type == 2)
	{
		// This exits the current directory

		char * findpath = strrchr(newdir, '/');
		if(findpath != dirfullpath)
		{
			findpath[0] = '\0';
			findpath = strrchr(newdir, '/');
			findpath[1] = '\0';
		}
	}
	else
	{
		
		// This enters the new directory
		size_t slen1 = strlen(newdir);
		size_t slen2 = strlen(entryIdx[idx]->name);
		char * extenddir = (char *)linearAlloc(slen1 + slen2 + 1);
		strcpy(extenddir, newdir);
		strcat(extenddir, &(entryIdx[idx]->name[1]));
		strcat(extenddir, "/");
		linearFree(newdir);
		newdir = extenddir;
	}

	bool ret = dirReadDir(newdir);
	linearFree(newdir);

	return ret;
}

void dirSetExt(char * extList)
{
	if(!dirInited)
		return;

	if(dirextlist)
	{
		linearFree(dirextlist);
		dirextlist = NULL;
	}

	size_t slen = strlen(extList);

	if(extList != NULL)
	{
		//if(slen > 0)
		{
			dirextlist = (char *)linearAlloc(slen + 1);
			strncpy(dirextlist, extList, slen);
			dirextlist[slen] = '\0';
		
		}
	}

}

bool dirInit()
{
	if(dirInited)
		return false;

	dirInited = true;
	dirSetExt("");
	dirReadDir("/");

	return true;
}

void dirDeInit()
{
	if(!dirInited)
		return;

	dirSetExt("");
	_dirclearlist();
	nEntries = 0;
	dirInited = false;
}