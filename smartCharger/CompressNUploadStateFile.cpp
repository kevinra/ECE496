/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.cpp

  
*/

#include "CompressNUploadStateFile.hpp"
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#define FTP_UPLOAD_TIMEOUT 3
#define BINARYLOCATION_TAR "/usr/bin/tar"
#define BINARYLOCATION_RM "/bin/rm"
#define SERVER_BASEURL "ftp://smdev:uoft@" SERVERURL "state/"
#define COMPRESSEDFILE_PREFIX "tarBall_"
#define SIZE_COMPRESSGROUP 5 // Number of state files grouped together and compressed
#define SIZE_UPLOADURLSTR 70
#define EXTENSIONOFFSET_7Z 3

CompressNUploadStateFile::CompressNUploadStateFile()
{
  m_curl = NULL;
  m_numTarCreated = 0;
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
  // while(1)
  // {
    DBG_OUT_MSG("Sweeping " << STATEFILE_LOCATION);

    DIR* dir;
    struct dirent* ent;
    m_numTarCreated = 0;
    m_numFileAdded = 0;

    if ( (dir = opendir(STATEFILE_LOCATION)) != NULL )
    {
      std::string fileNameArry[SIZE_COMPRESSGROUP];
      while ( (ent = readdir(dir)) != NULL )
      {
        DBG_OUT_MSG("Current file:" << ent->d_name);
        // A file that is currently used by vehicleState does not
        // start with 'S'
        if (*(ent->d_name) == 'S')
        {
          DBG_OUT_MSG("Piggybacked statefile found.");
          // Compress the group of files into tar ball and upload
          if (m_numFileAdded == SIZE_COMPRESSGROUP)
          {
            DBG_OUT_MSG("Number of files waiting for compression reached threshold.");
            compressNUploadGroup(fileNameArry);
          }
          DBG_OUT_MSG("Current file is added to the list.");
          char filePath[STATEFILE_FULLPATHSIZE];
          snprintf(filePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", ent->d_name);
          fileNameArry[m_numFileAdded] = filePath;
          m_numFileAdded++;
        }
        else if ( *(ent->d_name) != '.' && isTarFile(ent->d_name) )
        {
          DBG_OUT_MSG("Tar ball found, which is not uploaded yet.");
          uploadTarBall(ent->d_name);
        }
      }
      // Upload remaining not-yet uploaded file
      if (m_numFileAdded != 0)
      {
        DBG_OUT_MSG("Uploading remaining files.");
        compressNUploadGroup(fileNameArry);
      }
      closedir(dir);
    }
    else
    {
      ERR_MSG("could not open directory");
    }
    
    DBG_OUT_MSG("Scanning state file location done.");
  // }
  return NULL;
}


// Note: 7Zip files will be removed if uploading was successful.
// There is little chance that the uploading is failed so whenever
// we found *.7z file in the directory, just directly upload it.
bool CompressNUploadStateFile::isTarFile(char fileName[])
{
  char extTar[] = ".tar";
  int i_2 = 0;
  for (int i_1 = 0; fileName[i_1] != '\0'; i_1++)
  {
    if (fileName[i_1] == extTar[i_2])
    {
      i_1++;
      i_2++;
      while(fileName[i_1] != '\0' && extTar[i_2] != '\0')
      {
        if (fileName[i_1] != extTar[i_2])
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
void CompressNUploadStateFile::compressNUploadGroup(std::string fileNameArry[])
{
  char nameTarBall[STATEFILE_STRSIZE];
  char compressOutArg[STATEFILE_FULLPATHSIZE]; // Not exactly this size but whatever
  snprintf(nameTarBall, STATEFILE_STRSIZE, COMPRESSEDFILE_PREFIX "%d.tar.gz", m_numTarCreated);
  snprintf(compressOutArg, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", nameTarBall);

  int retVal = forkNRunCmd(CM_COMPRESS, compressOutArg, fileNameArry);
  m_numFileAdded = 0;
  if (retVal)
  {
    // In case incorrect tar ball is generated, remove it
    forkNRunCmd(CM_REMOVE, compressOutArg, NULL);
    // Flush the fileNameArry
    for (int i = 0; i < SIZE_COMPRESSGROUP; i++)
    {
      fileNameArry[i].clear();
    }
    return;
  }
  m_numTarCreated++;
  uploadTarBall(nameTarBall);

  // Remove state files that are contained in the 
  // uploaded compressed file
  forkNRunCmd(CM_REMOVE, NULL, fileNameArry);
  // Flush the fileNameArry
  for (int i = 0; i < SIZE_COMPRESSGROUP; i++)
  {
    fileNameArry[i].clear();
  }
  return;
}


// Note: it deletes .tar.gz file if upload is successful
void CompressNUploadStateFile::uploadTarBall(char nameTarBall[])
{
  char file_TarBallPath[STATEFILE_FULLPATHSIZE]; // the size of this has nothing to do with state file string size really
  char uploadURL[SIZE_UPLOADURLSTR];
  snprintf(file_TarBallPath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", nameTarBall);
  snprintf(uploadURL, SIZE_UPLOADURLSTR, SERVER_BASEURL "%s", nameTarBall);
  DBG_OUT_MSG(file_TarBallPath << " will be uploaded to " << uploadURL);

  FILE* hd_src = fopen(file_TarBallPath, "rb");
  if (hd_src == NULL)
  {
    ERR_MSG("Opening tar ball(" << file_TarBallPath << ") failed!");
    return;
  }
  curl_easy_setopt(m_curl, CURLOPT_URL, uploadURL);
  curl_easy_setopt(m_curl, CURLOPT_READDATA, hd_src);

  CURLcode res = curl_easy_perform(m_curl);
  if(res != CURLE_OK)
  {
    ERR_MSG("Tar ball uploading failed!\ncurl says:\n" << curl_easy_strerror(res) );
    return;
  }

  // Remove the tar ball
  forkNRunCmd(CM_REMOVE, file_TarBallPath, NULL);
  fclose(hd_src);
  return;
}

int CompressNUploadStateFile::forkNRunCmd(cmdMode cm, char arg1[], std::string arg2[])
{
  pid_t pID = fork();
  if (pID == 0) // Child process
  {
    if (cm == CM_REMOVE)
    {
      // If exec is successful, it would not return a value.
      // If statements are triggered only if there is some error.
      // However, not all the errors are handled by this.
      
      if (arg2 == NULL) // only single file should be removed.
      {
        DBG_OUT_MSG("Will remove " << arg1);
        if ( execl(BINARYLOCATION_RM, BINARYLOCATION_RM, arg1, NULL) )
        {
          ERR_MSG("rm command to remove" << arg1 << "failed!");
          exit(1);
        } 
      }
      else if (arg1 == NULL) // List of files that should be removed.
      {
        DBG_OUT_MSG("Will remove following files:\n" << 
                    arg2[0].c_str() << "\n" <<
                    arg2[1].c_str() << "\n" <<
                    arg2[2].c_str() << "\n" <<
                    arg2[3].c_str() << "\n" <<
                    arg2[4].c_str()
                   );

        if ( execl(BINARYLOCATION_RM, BINARYLOCATION_RM,
             arg2[0].c_str(), arg2[1].c_str(), arg2[2].c_str(), arg2[3].c_str(), arg2[4].c_str(),
             NULL) )
        {
          ERR_MSG("rm command to remove following files failed!\n" <<
                  arg2[0].c_str() << "\n" <<
                  arg2[1].c_str() << "\n" <<
                  arg2[2].c_str() << "\n" <<
                  arg2[3].c_str() << "\n" <<
                  arg2[4].c_str()
                 );
          exit(1);
        }
      }
      else
      {
        ERR_MSG("Cannot proceed rm commmand\nNeither of arg1 nor arg2 are supplied!");
        exit(1);
      }
    }
    else if (cm == CM_COMPRESS)
    {
      if (arg2 == NULL) // Second argument must be presented
      {
        ERR_MSG("Second argument for tar compression command is NULL!");
        exit(1);
      }
      DBG_OUT_MSG("Following files will be compressed into " << 
                  arg1 << ":\n" <<
                  arg2[0].c_str() << "\n" <<
                  arg2[1].c_str() << "\n" <<
                  arg2[2].c_str() << "\n" <<
                  arg2[3].c_str() << "\n" <<
                  arg2[4].c_str()
                 );

      if ( execl(BINARYLOCATION_TAR, BINARYLOCATION_TAR, "-cpzf", arg1,
           arg2[0].c_str(), arg2[1].c_str(), arg2[2].c_str(), arg2[3].c_str(), arg2[4].c_str(),
           NULL) )
      {
        // For now, cannot find a way to pass less than SIZE_COMPRESSGROUP
        // without triggering the error output
        ERR_MSG("tar ball compress command failed!");
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
  // return WEXITSTATUS(childRetVal); // For future improvement
  return 0;
}
