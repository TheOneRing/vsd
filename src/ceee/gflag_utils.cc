// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of the launch with gflags function.

// From https://github.com/madecoste/ceee

#include "gflag_utils.h"

#include <shlwapi.h>
#include <winternl.h>

namespace testing {

HRESULT WriteProcessGFlags(HANDLE process, DWORD gflags) {
  PROCESS_BASIC_INFORMATION basic_info = {};
  ULONG return_len = 0;
  NTSTATUS status =
      NtQueryInformationProcess(process,
                                ProcessBasicInformation,
                                &basic_info,
                                sizeof(basic_info),
                                &return_len);
  if (0 != status)
    return HRESULT_FROM_NT(status);
  else if (return_len != sizeof(basic_info))
    return E_UNEXPECTED;

  LPVOID gflags_location =
      reinterpret_cast<char*>(basic_info.PebBaseAddress) + kGFlagsPEBOffset;

  SIZE_T num_written = 0;
  if (!::WriteProcessMemory(process,
                            gflags_location,
                            &gflags,
                            sizeof(gflags),
                            &num_written) ||
      sizeof(gflags) != num_written)
    return HRESULT_FROM_WIN32(::GetLastError());

  return S_OK;
}

HRESULT CreateProcessWithGFlags(LPCWSTR application_name,
                                LPWSTR command_line,
                                LPSECURITY_ATTRIBUTES process_attributes,
                                LPSECURITY_ATTRIBUTES thread_attributes,
                                BOOL inherit_handles,
                                DWORD creation_flags,
                                LPVOID environment,
                                LPCWSTR current_directory,
                                LPSTARTUPINFOW startup_info,
                                LPPROCESS_INFORMATION proc_info,
                                DWORD gflags) {
  // Create the process, but make sure it's suspended at creation.
  BOOL created = ::CreateProcess(application_name,
                                 command_line,
                                 process_attributes,
                                 thread_attributes,
                                 inherit_handles,
                                 creation_flags | CREATE_SUSPENDED,
                                 environment,
                                 current_directory,
                                 startup_info,
                                 proc_info);
  if (!created)
    return HRESULT_FROM_WIN32(::GetLastError());

  // If we succeeded, go through and write the new process'
  // PEB with the requested gflags.
  HRESULT hr = WriteProcessGFlags(proc_info->hProcess, gflags);
  if (FAILED(hr)) {
    // We failed to write the gflags value, clean up and bail.
    ::TerminateProcess(proc_info->hProcess, 0);
    ::CloseHandle(proc_info->hProcess);
    ::CloseHandle(proc_info->hThread);

    return hr;
  }

  // Resume the main thread unless suspended creation was requested.
  if (!(creation_flags & CREATE_SUSPENDED))
    ::ResumeThread(proc_info->hThread);

  return hr;
}


}  //  namespace testing
