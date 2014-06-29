/**
 * Susie plug-in: HSP DPM archive extractor
 * written by gocha, feel free to redistribute
 * 
 * based on spi00am_ex.cpp by Shimitei:
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 * 
 * Susie�v���O�C����UNDPM32.DLL�̍�肪�t�ł��邱�Ƃɉ����A
 * �e�폈���̎蔲�������x�ቺ�̗v���ƂȂ��Ă��邩������܂���B
 */

#include <windows.h>
#include <stdlib.h>
#include <memory.h>
#include "spi00am.h"
#include "axdpm.h"
#include "undpm32/undpm32.h"

/**
 * �G���g���|�C���g
 */
BOOL APIENTRY SpiEntryPoint(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

/**
 * �t�@�C���擪2KB�ȓ�����Ή��t�H�[�}�b�g�����f
 * (���f�Ɏg�p����T�C�Y�̓w�b�_�t�@�C���Œ�`)
 * �t�@�C���������f�ޗ��Ƃ��ēn����Ă���݂���
 */
BOOL IsSupportedEx(char *filename, char *data)
{
  const BYTE dpmHdrSig[4] = { 'D', 'P', 'M', 'X' };
  const BYTE exeHdrSig[4] = { 'M', 'Z', 0x90, 0x00 };

  // �擪�o�C�g��݂̂̊ȈՃ`�F�b�N���s��
  if(memcmp(data, dpmHdrSig, 4) == 0)
  {
    return TRUE;
  }
#ifdef UNDPM32_ALLOWEXEFILE
  else if(memcmp(data, exeHdrSig, 4) == 0)
  {
    return TRUE;
  }
#endif
  else
  {
    return FALSE;
  }
}

/**
 * �A�[�J�C�u���S�t�@�C���̏��擾
 * filename�̈ʒulen��擪�Ƃ݂Ȃ��ăA�[�J�C�u�Ƀt�@�C�������i�[
 * �������Ƃ������̈��������̂��߁A�����I��len��0�ȊO�Ŏg���܂���
 */
int GetArchiveInfoEx(LPSTR filename, long len, HLOCAL *lphInf)
{
  int result = SPI_ALL_RIGHT;
  HDPM dpm;

  dpm = UnDpmOpenArchive(filename, (DWORD) len);
  if(dpm)
  {
    DWORD nFiles;
    fileInfo* pInfo;

    // �t�@�C�������擾
    nFiles = UnDpmGetFileCount(dpm);

    // �ԋp����t�@�C���������蓖�Ă� (�t�@�C����+1��, �������K�{)
    pInfo = (fileInfo*) LocalAlloc(LPTR, sizeof(fileInfo) * (nFiles+1));
    if(pInfo)
    {
      DWORD fileId;

      *lphInf = (HLOCAL) pInfo;
      // �e�t�@�C����A���I�ɏ���
      for(fileId = 1; fileId <= nFiles; fileId++)
      {
        // ���k�@: 7�����ȓ��̃��j�[�N�ȕ�����
        strcpy((char*) pInfo->method, "HSP DPM");
        // �ʒu: �t�@�C�������ʂ��A�����ɏ������邽�߂̌��ɂ���Ɨǂ�
        // ���{�v���O�C���ł͎d���Ȃ��AUNDPM32�p�̃t�@�C��ID������
        pInfo->position = fileId;
        // ���k���ꂽ�T�C�Y: �����k
        // �Ԃ��l�̓w�b�_�Ȃǂ̃T�C�Y���܂߂�ׂ��Ƃ����
        pInfo->compsize = UnDpmGetCompressedSize(dpm, fileId);
        // ���̃t�@�C���̃T�C�Y: �����k
        pInfo->filesize = UnDpmGetOriginalSize(dpm, fileId);
        // �t�@�C���̍X�V����: �L�^�Ȃ�
        pInfo->timestamp = 0;
        // ���΃p�X: �c���[�L�^�s��
        pInfo->path[0] = '\0';
        // �t�@�C����
        UnDpmGetFileNameA(dpm, fileId, pInfo->filename, 200);
        // CRC: �L�^�Ȃ�
        pInfo->crc = 0;

        pInfo++;
      }
    }
    else
    {
      result = SPI_NO_MEMORY;
    }
    UnDpmCloseArchive(dpm);
  }
  else
  {
    result = SPI_FILE_READ_ERROR;
  }
  return result;
}

/**
 * filename�ɂ���fileInfo�̃t�@�C����ǂݍ���
 * �f�R�[�h���ꂽ�t�@�C�����i�[������Ԃ�dest�ɓn��
 */
int GetFileEx(char *filename, HLOCAL *dest, fileInfo *pinfo,
    SPI_PROGRESS lpPrgressCallback, long lData)
{
  int result = SPI_ALL_RIGHT;
  HDPM dpm;

  dpm = UnDpmOpenArchive(filename, 0);
  if(dpm)
  {
    DWORD fileId = (DWORD) pinfo->position;
    DWORD dataSize = (DWORD) pinfo->filesize;
    LPBYTE data;

    // �t�@�C���o�b�t�@���m��
    data = (LPBYTE) LocalAlloc(LMEM_FIXED, dataSize);
    if(data)
    {
      // �t�@�C���ǂݍ���
      if(UnDpmExtractMem(dpm, fileId, data, dataSize))
      {
        *dest = (HLOCAL) data;
      }
      else
      {
        result = SPI_FILE_READ_ERROR;
      }
    }
    else
    {
      result = SPI_NO_MEMORY;
    }
    UnDpmCloseArchive(dpm);
  }
  else
  {
    result = SPI_FILE_READ_ERROR;
  }
  return result;
}
