#include "chromium_cookie.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

QString decryptChromiumCookieV10(const QByteArray &encrypted_value, const QString &host)
{
	if(host.isEmpty())
	{
		return QString();
	}

	QByteArray ciphertext=encrypted_value.mid(3);
	if(ciphertext.isEmpty() || ciphertext.size() % 16 != 0)
	{
		return QString();
	}

	unsigned char key[16];
	unsigned char iv[16];
	for(unsigned char &byte : iv)
	{
		byte=' ';
	}
	if(PKCS5_PBKDF2_HMAC_SHA1("peanuts", 7, reinterpret_cast<const unsigned char *>("saltysalt"), 9, 1, sizeof(key), key) != 1)
	{
		return QString();
	}

	EVP_CIPHER_CTX *context=EVP_CIPHER_CTX_new();
	if(context==nullptr)
	{
		OPENSSL_cleanse(key, sizeof(key));
		return QString();
	}

	QByteArray plaintext(ciphertext.size() + 16, 0);
	int written=0;
	int finalWritten=0;
	bool success=EVP_DecryptInit_ex(context, EVP_aes_128_cbc(), nullptr, key, iv)==1 &&
		EVP_DecryptUpdate(context, reinterpret_cast<unsigned char *>(plaintext.data()), &written,
				reinterpret_cast<const unsigned char *>(ciphertext.constData()), ciphertext.size())==1 &&
		EVP_DecryptFinal_ex(context, reinterpret_cast<unsigned char *>(plaintext.data()) + written, &finalWritten)==1;
	EVP_CIPHER_CTX_free(context);
	OPENSSL_cleanse(key, sizeof(key));
	if(!success)
	{
		return QString();
	}
	plaintext.resize(written + finalWritten);

	QByteArray hostBytes=host.toUtf8();
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char *>(hostBytes.constData()), hostBytes.size(), digest);
	if(plaintext.size() <= SHA256_DIGEST_LENGTH || CRYPTO_memcmp(plaintext.constData(), digest, SHA256_DIGEST_LENGTH)!=0)
	{
		return QString();
	}
	return QString::fromUtf8(plaintext.constData() + SHA256_DIGEST_LENGTH, plaintext.size() - SHA256_DIGEST_LENGTH);
}

QString decryptChromiumCookieV11(const QByteArray &encrypted_value, const QString &host)
{
	qWarning("Chromium v11 cookies are not supported");
	uint64_t WIP;
	encrypted_value+host;
	return QString();
}
