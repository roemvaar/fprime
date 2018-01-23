// ======================================================================
// \title  Errors.cpp
// \author bocchino, mereweth
// \brief  Implementation for Buffer Logger error tests
//
// \copyright
// Copyright (C) 2017 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
//
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ======================================================================

#include <stdlib.h>

#include "Errors.hpp"
#include "Os/ValidatedFile.hpp"

namespace Svc {

  namespace Errors {

    void Tester ::
      LogFileOpen(void)
    {
      // Remove buf directory
      (void) system("rm -rf buf");

      // Check initial state
      ASSERT_EQ(BufferLogger::File::Mode::CLOSED, this->component.m_file.mode);
      ASSERT_EVENTS_SIZE(0);

      // Send data
      this->sendComBuffers(3);

      // Check events
      // We expect only one because of the throttle
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_BL_LogFileOpenError_SIZE(1);
      ASSERT_EVENTS_BL_LogFileOpenError(
        0,
        Os::File::DOESNT_EXIST,
        this->component.m_file.name.toChar()
      );

      // Create buf directory and try again
      (void) system("mkdir buf");
      ASSERT_EQ(BufferLogger::File::Mode::CLOSED, this->component.m_file.mode);

      // Send data
      this->sendComBuffers(3);

      // Check events
      // We expect only one because of the throttle
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_BL_LogFileOpenError_SIZE(1);
      this->component.m_file.close();

      // Remove buf directory and try again
      (void) system("rm -rf buf");
      ASSERT_EQ(BufferLogger::File::Mode::CLOSED, this->component.m_file.mode);

      // Send data
      this->sendComBuffers(3);

      // Check events
      // We expect only one more because of the throttle
      ASSERT_EVENTS_SIZE(2);
      ASSERT_EVENTS_BL_LogFileOpenError_SIZE(2);

    }

    void Tester ::
      LogFileWrite(void)
    {
      ASSERT_EQ(BufferLogger::File::Mode::CLOSED, this->component.m_file.mode);
      ASSERT_EVENTS_SIZE(0);

      // Send data
      this->sendComBuffers(1);

      // Force close the file
      this->component.m_file.osFile.close();

      // Send data
      this->sendComBuffers(1);

      // Construct file name
      Fw::EightyCharString fileName;
      fileName.format(
          "%s%s%s",
          this->component.m_file.prefix.toChar(),
          this->component.m_file.baseName.toChar(),
          this->component.m_file.suffix.toChar()
      );

      // Check events
      // We should see one event because write errors are throttled
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_BL_LogFileWriteError_SIZE(1);
      ASSERT_EVENTS_BL_LogFileWriteError(
          0,
          Os::File::NOT_OPENED, // errornum
          sizeof(SIZE_TYPE), // bytesWritten
          sizeof(SIZE_TYPE), // bytesAttempted
          fileName.toChar() // file
      );

      // Make comlogger open a new file:
      this->component.m_file.mode = BufferLogger::File::Mode::CLOSED;
      this->component.m_file.open();

      // Try to write and make sure it succeeds
      // Send data
      this->sendComBuffers(3);

      // Expect no new errors
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_BL_LogFileWriteError_SIZE(1);

      // Force close the file from underneath the component
      component.m_file.osFile.close();

      // Send data
      this->sendComBuffers(3);

      // Check events
      // We should see one event because write errors are throttled
      ASSERT_EVENTS_SIZE(2);
      ASSERT_EVENTS_BL_LogFileWriteError_SIZE(2);
      ASSERT_EVENTS_BL_LogFileWriteError(
          1,
          Os::File::NOT_OPENED,
          sizeof(SIZE_TYPE),
          sizeof(SIZE_TYPE),
          fileName.toChar()
      );

    }

    void Tester ::
      LogFileValidation(void)
    {
      // Send data
      this->sendComBuffers(1);
      // Remove permission to buf directory
      (void) system("chmod -w buf");
      // Send close file command
      this->sendCmd_BL_CloseFile(0, 0);
      this->dispatchOne();
      // Check events
      ASSERT_EVENTS_SIZE(2);
      Fw::EightyCharString fileName;
      fileName.format(
          "%s%s%s",
          this->component.m_file.prefix.toChar(),
          this->component.m_file.baseName.toChar(),
          this->component.m_file.suffix.toChar()
      );
      ASSERT_EVENTS_BL_LogFileClosed(
          0,
          fileName.toChar()
      );
      Os::ValidatedFile validatedFile(fileName.toChar());
      const Fw::EightyCharString& hashFileName = validatedFile.getHashFileName();
      ASSERT_EVENTS_BL_LogFileValidationError(
          0,
          hashFileName.toChar(),
          Os::ValidateFile::VALIDATION_FILE_NO_PERMISSION
      );
      // Restore permission
      (void) system("chmod +w buf");
    }

  }

}
