/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.cpp

  
*/

#include "CompressNUploadStateFile.hpp"
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define FTP_UPLOAD_TIMEOUT 3
#define BINARYLOCATION_7ZIP "/usr/local/bin/7za"
#define BINARYLOCATION_RM "/bin/rm"
#define SERVER_BASEURL "ftp://smdev:uoft@smartcharger.zapto.org/stateFiles/"
#define COMPRESSEDFILE_PREFIX "7zcomp_"
#define SIZE_COMPRESSGROUP 5 // Number of state files grouped together and compressed
#define SIZE_COMPRESSFILELISTSTR SIZE_COMPRESSGROUP * STATEFILE_FULLPATHSIZE
#define SIZE_UPLOADURLSTR 70
#define EXTENSIONOFFSET_7Z 3

CompressNUploadStateFile::CompressNUploadStateFile()
{
  m_curl = NULL;
  m_num7ZipCreated = 0;
  m_numFileAdded = 0;
}

CompressNUploadStateFile::~CompressNUploadStateFile()
{
  if (m_curl)
  {
    curl_easy_cleanup(m_curl);
  }
}

int CompressNUploadStateFile::init()
{
  m_curl = curl_easy_init();
  if (m_curl)
  {
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_curl, CURLOPT_FTP_RESPONSE_TIMEOUT, FTP_UPLOAD_TIMEOUT);
  }
  else
  {
    ERR_MSG("curl ftp upload handler initialization failed!");
    return 1;
  }
  return 0;
}

void* CompressNUploadStateFile::run()
{
  while(1)
  {
    DIR* dir;
    struct dirent* ent;
    m_num7ZipCreated = 0;
    m_numFileAdded = 0;

    if ( (dir = opendir(STATEFILE_LOCATION)) != NULL )
    {
      char fileListStr[SIZE_COMPRESSFILELISTSTR];
      memset(fileListStr, 0, SIZE_COMPRESSFILELISTSTR);
      while ( (ent = readdir(dir)) != NULL )
      {
        // A file that is currently used by vehicleState does not
        // start with 'S'
        if (*(ent->d_name) == 'S')
        {
          // Compress the group of files with 7zip and upload
          if (m_numFileAdded == SIZE_COMPRESSGROUP)
          {
            compressNUploadGroup(fileListStr);
          }
          else
          {
            char filePath[STATEFILE_FULLPATHSIZE];
            snprintf(filePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", ent->d_name);
            strncat(fileListStr, filePath, STATEFILE_FULLPATHSIZE);
          }
        }
        else if ( is7zFile(ent->d_name) )
        {
          upload7zFile(ent->d_name);
        }
      }
      // Upload remaining not-yet uploaded file
      if (m_numFileAdded != 0)
      {
        compressNUploadGroup(fileListStr);
      }
      closedir(dir);
    }
    else
    {
      ERR_MSG("could not open directory");
    }
    
    DBG_OUT_MSG("Scanning state file location done.");
  }
  return NULL;
}


// Note: 7Zip files will be removed if uploading was successful.
// There is little chance that the uploading is failed so whenever
// we found *.7z file in the directory, just directly upload it.
bool CompressNUploadStateFile::is7zFile(char fileName[])
{
  char ext7z[] = ".7z";
  int i_2 = 0;
  for (int i_1 = 0; fileName[i_1] != '\0'; i_1++)
  {
    if (fileName[i_1] == ext7z[i_2])
    {
      i_1++;
      i_2++;
      while(fileName[i_1] != '\0' && ext7z[i_2] != '\0')
      {
        if (fileName[i_1] != ext7z[i_2])
        {
          return false;
        }
        i_1++;
        i_2++;
      }
      return true;
    }
  }
  return false;
}


// Note that this deletes state files and compressed file if 
// successfully uploaded.
void CompressNUploadStateFile::compressNUploadGroup(char fileListStr[])
{
  char name7z[STATEFILE_STRSIZE];
  char compressOutArg[STATEFILE_FULLPATHSIZE]; // Not exactly this size but whatever
  snprintf(name7z, STATEFILE_STRSIZE, COMPRESSEDFILE_PREFIX "%d.7z", m_num7ZipCreated);
  snprintf(compressOutArg, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", name7z);

  int retVal = forkNRunCmd(CM_COMPRESS, compressOutArg, fileListStr);
  m_numFileAdded = 0;
  if (retVal)
  {
    // In case incorrect 7z file is generated, remove it
    forkNRunCmd(CM_REMOVE, compressOutArg, NULL);
    memset(fileListStr, 0, SIZE_COMPRESSFILELISTSTR);
    ERR_MSG("7Zip compression failed!");
    return;
  }
  m_num7ZipCreated++;
  upload7zFile(name7z);

  // Remove state files that are contained in the 
  // uploaded compressed file
  forkNRunCmd(CM_REMOVE, fileListStr, NULL);
  memset(fileListStr, 0, SIZE_COMPRESSFILELISTSTR);
  return;
}


// Note: it deletes .7z file if upload is successful
void CompressNUploadStateFile::upload7zFile(char name7z[])
{
  char file_7zPath[STATEFILE_FULLPATHSIZE]; // the size of this has nothing to do with state file string size really
  char uploadURL[SIZE_UPLOADURLSTR];
  snprintf(file_7zPath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", name7z);
  snprintf(uploadURL, SIZE_UPLOADURLSTR, SERVER_BASEURL "%s", name7z);
  FILE* hd_src = fopen(file_7zPath, "rb");
  if (hd_src == NULL)
  {
    ERR_MSG("Opening compressed file(" << file_7zPath << ") failed!");
    return;
  }
  curl_easy_setopt(m_curl, CURLOPT_URL, uploadURL);
  curl_easy_setopt(m_curl, CURLOPT_READDATA, hd_src);

  CURLcode res = curl_easy_perform(m_curl);
  if(res != CURLE_OK)
  {
    ERR_MSG("Compressed file uploading failed!\ncurl says:\n" << curl_easy_strerror(res) );
    return;
  }

  // Remove 7z file
  forkNRunCmd(CM_REMOVE, file_7zPath, NULL);
  fclose(hd_src);
  return;
}

int CompressNUploadStateFile::forkNRunCmd(cmdMode cm, char arg1[], char arg2[])
{
  pid_t pID = fork();
  if (pID == 0) // Child process
  {
    if (cm == CM_REMOVE)
    {
      // If exec is successful, it would not return a value, it will trigger the if statement
      // only if there is some error. However, not all the errors are handled by this.
      if ( execl(BINARYLOCATION_RM, BINARYLOCATION_RM, arg1, NULL) )
      {
        ERR_MSG("rm command to remove" << arg1 << "failed!");
        exit(1);
      }
    }
    else if (cm == CM_COMPRESS)
    {
      if (arg2 == NULL) // Seconc argument must be presented
      {
        ERR_MSG("Second argument for 7z compress command is NULL!");
        exit(1);
      }
      if ( execl(BINARYLOCATION_7ZIP, BINARYLOCATION_7ZIP, "a", "-mx=9", arg1, arg2, NULL) )
      {
        ERR_MSG("7z compress command failed!");
        exit(1);
      }
    }
    else
    {
      ERR_MSG("Unknown cmdMode!");
      exit(1);
    }
    exit(0);
  }
  else if (pID < 0) // fork() failed
  {
    #ifdef DEBUG
    if (cm == CM_REMOVE)
    {
      ERR_MSG("fork() with CM_REMOVE failed!");
    }
    else if (cm == CM_COMPRESS)
    {
      ERR_MSG("fork() with CM_COMPRESS failed!");
    }
    #endif
  }
  // Return whatever the child thread returned
  int childRetVal;
  waitpid(pID, &childRetVal, 0);
  kill(pID, SIGINT);
  return WEXITSTATUS(childRetVal);
}