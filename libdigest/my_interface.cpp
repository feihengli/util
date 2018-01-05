#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include "Base64.hh"
#include "strDup.hh"
#include "DigestAuthentication.hh"
#include "my_interface.h"

char* MY_Base64Encode(unsigned char* _au8Buf, unsigned int _u32BufSize)
{
	char* szRet;
	char* szBase64 = base64Encode((char const*)_au8Buf, (unsigned)_u32BufSize);

	szRet = (char*)malloc(strlen(szBase64) + 1);
	strcpy(szRet, szBase64);

	delete[] szBase64;
	return szRet;
}

static char* createAuthenticatorString(Authenticator& auth, char const* cmd, char const* url) {
//  Authenticator& auth = fCurrentAuthenticator; // alias, for brevity
  if (auth.realm() != NULL && auth.username() != NULL && auth.password() != NULL) {
    // We have a filled-in authenticator, so use it:
    char* authenticatorStr;
    if (auth.nonce() != NULL) { // Digest authentication
      char const* const authFmt =
	"Authorization: Digest username=\"%s\", realm=\"%s\", "
	"nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n";
      char const* response = auth.computeDigestResponse(cmd, url);
      unsigned authBufSize = strlen(authFmt)
	+ strlen(auth.username()) + strlen(auth.realm())
	+ strlen(auth.nonce()) + strlen(url) + strlen(response);
      authenticatorStr = new char[authBufSize];
      sprintf(authenticatorStr, authFmt,
	      auth.username(), auth.realm(),
	      auth.nonce(), url, response);
      auth.reclaimDigestResponse(response);
    } else { // Basic authentication
      char const* const authFmt = "Authorization: Basic %s\r\n";

      unsigned usernamePasswordLength = strlen(auth.username()) + 1 + strlen(auth.password());
      char* usernamePassword = new char[usernamePasswordLength+1];
      sprintf(usernamePassword, "%s:%s", auth.username(), auth.password());

      char* response = base64Encode(usernamePassword, usernamePasswordLength);
      unsigned const authBufSize = strlen(authFmt) + strlen(response) + 1;
      authenticatorStr = new char[authBufSize];
      sprintf(authenticatorStr, authFmt, response);
      delete[] response; delete[] usernamePassword;
    }

    return authenticatorStr;
  }

  // We don't have a (filled-in) authenticator.
  return strDup("");
}

char* MY_Authrization(char* _szCmd, char* _szUrl,
		char* _szUsr, char* _szPwd,
		char* _szRealm, char* _szNonce)
{
	char* szRet;
	int s32Ret;
	char* szTmp;
	char* szUsr;
	char* szPwd;
	char* szRealm;
	char* szNonce;
	Authenticator fCurrentAuthenticator;

	if(_szUsr == NULL || strlen(_szUsr) == 0)
	{
		szUsr = NULL;
	}
	else
	{
		szUsr = _szUsr;
	}

	if(_szPwd == NULL || strlen(_szPwd) == 0)
	{
		szPwd = NULL;
	}
	else
	{
		szPwd = _szPwd;
	}

	if(_szRealm == NULL || strlen(_szRealm) == 0)
	{
		szRealm = NULL;
	}
	else
	{
		szRealm = _szRealm;
	}

	if(_szNonce == NULL || strlen(_szNonce) == 0)
	{
		szNonce = NULL;
	}
	else
	{
		szNonce = _szNonce;
	}

	fCurrentAuthenticator.setUsernameAndPassword(szUsr, szPwd);
	fCurrentAuthenticator.setRealmAndNonce(szRealm, szNonce);

	szTmp = createAuthenticatorString(fCurrentAuthenticator,
		_szCmd,
		_szUrl);

	s32Ret = strlen(szTmp);
	szRet = (char*)malloc(s32Ret + 1);
	sprintf(szRet, szTmp);
	delete[] szTmp;
	return szRet;
}
