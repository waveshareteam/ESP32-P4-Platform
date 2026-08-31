# Security Policy

[中文版本](SECURITY_CN.md)

## Private Reporting Channel

Do not publish vulnerability details in a GitHub issue, discussion, pull
request, log, or screenshot.

Submit a private ticket to the Waveshare Support Team through the official
support portal: <https://service.waveshare.com>. Identify the repository as
`waveshareteam/ESP32-P4-Platform` and mark the request as a security report.
This repository does not currently accept GitHub private vulnerability reports,
so use the support portal rather than the public issue tracker.

Include only the information needed to investigate:

- Affected example, component, firmware package, or configuration path.
- A clear description, impact, and affected board/revision.
- Reproduction steps or a minimal proof of concept.
- Relevant version and commit SHA.
- Whether the issue or exploit is already public.

Remove unrelated credentials, tokens, personal data, private network details,
unique device identifiers, and local paths. If a secret is essential to explain
the issue, first ask the support team how to transfer it safely.

## Scope and Support

This policy covers first-party code and configuration maintained in this
repository. Vulnerabilities originating in a bundled library or managed
component may also need coordinated reporting to that upstream maintainer.

The default branch receives current fixes. Historical snapshots, release
artifacts, and downstream forks are supported only when explicitly stated.
No public response-time or remediation-time commitment is made by this file;
keep the private ticket reference for follow-up.
