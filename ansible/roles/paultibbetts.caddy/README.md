Caddy
=========

An Ansible role to install and configure Caddy. Defaults to installing from the
official apt repository, with optional support for downloading a custom binary
(e.g., with plugins).

Requirements
------------

- Debian/Ubuntu are supported by the apt install path.
- Systemd is required for service management.

Role Variables
--------------

Install method:

- `caddy_install_method` (default: `apt`)
  - `apt` installs from the official Caddy apt repo
  - `download` downloads a binary from a URL
- `caddy_state` (default: `present`; set to `absent` to uninstall)
- `caddy_purge` (default: `false`; when `true`, removes config/log dirs and user/group on uninstall)

Download options (used when `caddy_install_method` is `download`):

- `caddy_download_url` (optional): full URL to a binary; if empty, the role
  builds a URL using the download API variables below
- `caddy_download_base_url` (default: `https://caddyserver.com/api/download`)
- `caddy_download_os` (default: `linux`)
- `caddy_download_arch` (default derived from `ansible_facts["architecture"]`)
- `caddy_plugins` (default: `[]`) list/dict/string of plugin module paths
- `caddy_download_checksum` (optional): checksum for the binary (`sha256:<hex>`)
- `caddy_download_force` (default: `false`)
- `caddy_download_tmp_path` (default: `/tmp/caddy`)
- `caddy_cleanup_download` (default: `true`)

Service/config:

- `caddy_manage_service` (default: `true`)
- `caddy_service_state` (default: `started`)
- `caddy_service_enabled` (default: `true`)
- `caddy_bin_path` (default: `/usr/bin/caddy` for apt installs)
- `caddy_service_name` (default: `caddy`)
- `caddy_user_name` / `caddy_group_name` (default: `caddy`)
- `caddy_user_home_directory` (default: `/var/lib/caddy`)
- `caddy_log_path` (default: `/var/log/caddy`; convenience directory if your Caddyfile logs to files)
- `caddy_config_path` (default: `/etc/caddy`)
- `caddy_caddyfile_template` (default: empty; opt-in, you provide the template)
- `caddy_systemd_unit_template` (default: `caddy.service.j2`)
- `caddy_manage_systemd_env_file` (default: `false`)
- `caddy_systemd_env_file_path` (default: `/etc/caddy/caddy.env`)
- `caddy_systemd_env` (default: `{}`; key/value env vars written to the env file when managed)
- `caddy_env_vars` (default: `[]`)
- `caddy_ambient_capabilities` (default: `[CAP_NET_BIND_SERVICE]`)

Notes / Caveats
---------------

- Download installs are only re-fetched when the binary is missing or when
  `caddy_download_force: true` is set. If you change `caddy_plugins` or
  `caddy_download_url`, you may want to set `caddy_download_force: true`
  (or provide `caddy_download_checksum`).
- You can use `caddy_download_url` to point at a specific prebuilt binary
  (for example, a pinned GitHub release asset or a custom build).
- Overrides like `caddy_user_name`, `caddy_group_name`, `caddy_service_name`,
  and `caddy_bin_path` are fully applied only for the `download` method. The
  `apt` method uses the package-managed systemd unit.
- For integrity verification of downloaded binaries, set
  `caddy_download_checksum`.
- When service management is enabled (`caddy_manage_service: true`), the role
  only attempts to start/reload Caddy for `apt` installs or when a
  `caddy_caddyfile_template` is provided (since the service needs a config).
- The Caddyfile template task runs with `no_log` to avoid leaking secrets in
  diffs/log output.
- For secrets, prefer `caddy_systemd_env` (written to a root-only env file) over
  `caddy_env_vars`, which are embedded directly in the unit file and are
  world-readable.

Example Playbook
----------------

Basic apt install:

```
- hosts: servers
  become: true
  roles:
    - role: caddy
```

Download a custom build with plugins:

```
- hosts: servers
  become: true
  roles:
    - role: caddy
      vars:
        caddy_install_method: download
        caddy_plugins:
          - github.com/caddy-dns/cloudflare
```

Download a specific prebuilt binary via URL (pinned release or custom build):

```
- hosts: servers
  become: true
  roles:
    - role: caddy
      vars:
        caddy_install_method: download
        caddy_download_url: "https://example.com/path/to/caddy"
        caddy_download_checksum: "sha256:0123456789abcdef..."
```

Use a Caddyfile template:

```
- hosts: servers
  become: true
  roles:
    - role: caddy
      vars:
        caddy_caddyfile_template: "{{ playbook_dir }}/templates/Caddyfile.j2"
```

Pass secrets to Caddy (via root-only env file) and set non-secret env vars directly:

```
- hosts: servers
  become: true
  roles:
    - role: caddy
      vars:
        caddy_caddyfile_template: "{{ playbook_dir }}/templates/Caddyfile.j2"
        caddy_install_method: download
        caddy_plugins:
          - github.com/caddy-dns/cloudflare
        # Enable and populate an optional systemd EnvironmentFile.
        caddy_manage_systemd_env_file: true
        caddy_systemd_env:
          CF_API_TOKEN: "{{ vault_cf_api_token }}"
        caddy_env_vars:
          - "CADDY_EXAMPLE=1"
```


Molecule Scenarios
------------------

- `default`: apt install only
- `cloudflare`: download build with Cloudflare DNS plugin
- `caddyfile`: apt install + Caddyfile end-to-end check
- `debian`: apt install on Debian 11/12 (bullseye/bookworm)
- `ubuntu22_arm`: ARM64 Ubuntu 22 + download build + Cloudflare plugin + Caddyfile

Prereqs:

- Docker running locally (Molecule uses the Docker driver).
- Python + `uv` installed.
- Collections installed:
  - `uv run ansible-galaxy collection install -r molecule/requirements.yml`

Run a scenario:

```
uv sync
uv run molecule test -s default
uv run molecule test -s cloudflare
uv run molecule test -s caddyfile
uv run molecule test -s debian
uv run molecule test -s ubuntu22_arm
```

If you use Colima on macOS, prefix with:

```
DOCKER_HOST=unix://$HOME/.colima/default/docker.sock
```

License
-------

MIT

Author Information
------------------

Paul Tibbetts
