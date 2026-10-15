// Common.h - C++/CLI 프록시 클래스들이 공유하는 헬퍼 모음.
#pragma once

#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_PrinterOStream.hxx>

#include <vcclr.h>
#include <exception>

namespace OcctProxy
{
  /// OCCT 알고리즘이 실패할 때 .NET 호출자에게 던져지는 예외.
  public ref class OcctException : public System::Exception
  {
  public:
    OcctException(System::String^ message) : System::Exception(message) {}
  };

  /// 네이티브 OCCT 라이브러리의 정적 정보 및 전역 설정.
  public ref class OcctInfo abstract sealed
  {
  public:
    /// 프록시에 컴파일된 OCCT 버전 문자열 (예: "8.0.1").
    static property System::String^ Version
    {
      System::String^ get() { return gcnew System::String(OCC_VERSION_COMPLETE); }
    }

    /// OCCT 자체 콘솔 출력(예: stdout으로 나오는 STEP 변환 통계)을 켜거나 끈다.
    static void SetConsoleOutput(bool enabled)
    {
      const Handle(Message_Messenger)& aMessenger = Message::DefaultMessenger();
      aMessenger->RemovePrinters(STANDARD_TYPE(Message_PrinterOStream));
      if (enabled)
      {
        aMessenger->AddPrinter(new Message_PrinterOStream());
      }
    }
  };

  // ---- 문자열 변환 -----------------------------------------------------------

  /// System.String (UTF-16) -> TCollection_AsciiString (UTF-8) 변환.
  inline TCollection_AsciiString ToAscii(System::String^ theString)
  {
    if (theString == nullptr || theString->Length == 0)
    {
      return TCollection_AsciiString();
    }
    pin_ptr<const wchar_t> aPinned = PtrToStringChars(theString);
    return TCollection_AsciiString(static_cast<const wchar_t*>(aPinned));
  }

  /// TCollection_AsciiString (UTF-8) -> System.String 변환.
  inline System::String^ ToManaged(const TCollection_AsciiString& theString)
  {
    TCollection_ExtendedString anExt(theString, true); // UTF-8 -> UTF-16 변환
    return gcnew System::String(anExt.ToWideString());
  }

  inline System::String^ ToManaged(const char* theString)
  {
    return ToManaged(TCollection_AsciiString(theString));
  }
}

// OCCT 호출 래핑: 네이티브 예외를 관리 코드의 OcctException으로 변환한다.
#define OCCT_TRY try {
#define OCCT_CATCH                                                                   \
  }                                                                                  \
  catch (const Standard_Failure& aFailure)                                           \
  {                                                                                  \
    const char* aMsg = aFailure.what();                                              \
    throw gcnew OcctProxy::OcctException(                                            \
      gcnew System::String(aMsg != nullptr && *aMsg != '\0' ? aMsg : "OCCT failure")); \
  }                                                                                  \
  catch (const std::exception& anStd)                                                \
  {                                                                                  \
    throw gcnew OcctProxy::OcctException(gcnew System::String(anStd.what()));        \
  }
