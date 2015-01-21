// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_TPM_UTILITY_H_
#define TRUNKS_TPM_UTILITY_H_

#include <string>

#include <base/macros.h>
#include <chromeos/chromeos_export.h>

#include "trunks/tpm_generated.h"

namespace trunks {

// These handles will be used by TpmUtility to create storage root keys.
const TPMI_DH_PERSISTENT kRSAStorageRootKey = PERSISTENT_FIRST;
const TPMI_DH_PERSISTENT kECCStorageRootKey = PERSISTENT_FIRST + 1;
const TPMI_DH_PERSISTENT kSaltingKey = PERSISTENT_FIRST + 2;

// An interface which provides convenient methods for common TPM operations.
class CHROMEOS_EXPORT TpmUtility {
 public:
  enum AsymmetricKeyUsage {
    kDecryptKey,
    kSignKey,
    kDecryptAndSignKey
  };

  TpmUtility() {}
  virtual ~TpmUtility() {}

  // Synchronously performs a TPM startup sequence and self tests. Typically
  // this is done by the platform firmware. Returns the result of the startup
  // and self-tests or, if already started, just the result of the self-tests.
  virtual TPM_RC Startup() = 0;

  // This method removes all TPM context associated with a specific Owner.
  // As part of this process, it resets the SPS to a new random value, and
  // clears ownerAuth, endorsementAuth and lockoutAuth.
  // NOTE: This method needs to be called before InitializeTPM.
  virtual TPM_RC Clear() = 0;

  // Synchronously performs a TPM shutdown operation. It should always be
  // successful.
  virtual void Shutdown() = 0;

  // Synchronously prepares a TPM for use by Chromium OS. Typically this is done
  // by the platform firmware and, in that case, this method has no effect.
  virtual TPM_RC InitializeTpm() = 0;

  // Synchronously takes ownership of the TPM with the given passwords as
  // authorization values.
  virtual TPM_RC TakeOwnership(const std::string& owner_password,
                               const std::string& endorsement_password,
                               const std::string& lockout_password) = 0;

  // Stir the tpm random generation module with some random entropy data.
  virtual TPM_RC StirRandom(const std::string& entropy_data) = 0;

  // This method returns |num_bytes| of random data generated by the tpm.
  virtual TPM_RC GenerateRandom(size_t num_bytes, std::string* random_data) = 0;

  // This method extends the pcr specified by |pcr_index| with the SHA256
  // hash of |extend_data|. The exact action performed is
  // TPM2_PCR_Extend(Sha256(extend_data));
  virtual TPM_RC ExtendPCR(int pcr_index, const std::string& extend_data) = 0;

  // This method reads the pcr specified by |pcr_index| and returns its value
  // in |pcr_value|. NOTE: it assumes we are using SHA256 as our hash alg.
  virtual TPM_RC ReadPCR(int pcr_index, std::string* pcr_value) = 0;

  // This method performs an encryption operation using a LOADED RSA key
  // referrenced by its handle |key_handle|. The |plaintext| is then encrypted
  // to give us the |ciphertext|. |scheme| refers to the encryption scheme
  // to be used. By default keys use OAEP, but can also use TPM_ALG_RSAES.
  virtual TPM_RC AsymmetricEncrypt(TPM_HANDLE key_handle,
                                   TPM_ALG_ID scheme,
                                   TPM_ALG_ID hash_alg,
                                   const std::string& plaintext,
                                   std::string* ciphertext) = 0;

  // This method performs a decyption operating using a loaded RSA key
  // referenced by its handle |key_handle|. The |ciphertext| is then decrypted
  // to give us the |plaintext|. We need |password| to authorize use of the
  // key. |scheme| refers to the decryption scheme used. By default it is
  // OAEP, but TPM_ALG_RSAES can be specified.
  virtual TPM_RC AsymmetricDecrypt(TPM_HANDLE key_handle,
                                   TPM_ALG_ID scheme,
                                   TPM_ALG_ID hash_alg,
                                   const std::string& password,
                                   const std::string& ciphertext,
                                   std::string* plaintext) = 0;

  // This method takes an unrestricted signing key referenced by |key_handle|
  // and uses it to sign the value of |digest|. The signature produced is
  // returned using the |signature| argument. We use the |password| argument
  // to authorize use of the key. |scheme| is used to specify the signature
  // scheme used. By default it is TPM_ALG_RSASSA, but TPM_ALG_RSAPPS can
  // be specified. hash_alg is the algorithm used in the signing operation.
  // It is by default TPM_ALG_SHA256.
  virtual TPM_RC Sign(TPM_HANDLE key_handle,
                      TPM_ALG_ID scheme,
                      TPM_ALG_ID hash_alg,
                      const std::string& password,
                      const std::string& digest,
                      std::string* signature) = 0;

  // This method verifies that the signature produced on the digest was
  // performed by |key_handle|. |scheme| and |hash| refer to the signature
  // scheme used to sign |digest| and produce the signature. This value is by
  // default TPM_ALG_RSASSA with TPM_ALG_SHA256 but can take the value of
  // TPM_ALG_RSAPPS with other hash algorithms supported by the tpm.
  // Returns TPM_RC_SUCCESS when the signature is correct.
  virtual TPM_RC Verify(TPM_HANDLE key_handle,
                        TPM_ALG_ID scheme,
                        TPM_ALG_ID hash_alg,
                        const std::string& digest,
                        const std::string& signature) = 0;

  // This method creates an RSA key. It creates a 2048 bit RSA key with
  // public exponent of 0x10001. |key_type| determines whether the key is
  // a signing key, a decryption key, or both. The |password| parameter
  // is used as the authorization for the created key. The created key
  // is then loaded and its handle is returned as |key_handle|. The out
  // argument |key_blob| can be used to load the key in the future.
  virtual TPM_RC CreateAndLoadRSAKey(AsymmetricKeyUsage key_type,
                                     const std::string& password,
                                     TPM_HANDLE* key_handle,
                                     std::string* key_blob) = 0;

  // This method uses the TPM to generates an RSA key of type |key_type|.
  // |modulus_bits| is used to specify the size of the modulus, and
  // |public_exponent| specifies the exponent of the key. After this function
  // terminates, |key_blob| contains a key blob that can be loaded into the TPM.
  virtual TPM_RC CreateRSAKeyPair(AsymmetricKeyUsage key_type,
                                  int modulus_bits,
                                  uint32_t public_exponent,
                                  const std::string& password,
                                  std::string* key_blob) = 0;

  // This method loads a pregenerated TPM key into the TPM. |key_blob| contains
  // the blob returned by a key creation function. The loaded key's handle is
  // returned using |key_handle|.
  virtual TPM_RC LoadKey(const std::string& key_blob,
                         TPM_HANDLE* key_handle) = 0;

  // This function sets |name| to the name of the object referenced by
  // |handle|. This function only works on Transient and Permanent objects.
  virtual TPM_RC GetKeyName(TPM_HANDLE handle, std::string* name) = 0;

  // This function returns the public area of a handle in the tpm.
  virtual TPM_RC GetKeyPublicArea(TPM_HANDLE handle,
                                  TPM2B_PUBLIC* public_data) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TpmUtility);
};

}  // namespace trunks

#endif  // TRUNKS_TPM_UTILITY_H_
