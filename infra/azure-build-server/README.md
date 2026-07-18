# Azure clang build server

Terraform for the box that builds the std::embed-patched clang/LLVM toolchain
(via [`scripts/build-clang/build.sh`](../../scripts/build-clang/build.sh)) from
the fork: **github.com/alexios-angel/llvm-project**, branch **`std-embed`**
(upstream base `ced9fa358` + the full std::embed / `#depend` commit). That
branch is where further std::embed work happens — compile-time file writing /
compile-time IO experiments land there, get pushed, and this server rebuilds.

- **Standard_D8as_v7** by default: 8 vCPU / 32 GiB, ~$0.31/hr pay-as-you-go
  (eastus), 256 GB Premium SSD. Unlike the ctbrowser PCH bake, an LLVM build
  saturates every core — a full clang+lld build is ~2 hours here; ccache
  (30 GB) makes patch-iteration rebuilds far cheaper. If you raise the
  subscription's eastus regional-cores quota to >=18 (it was 10), switch to
  `-var vm_size=Standard_D16as_v7` (16/64, ~$0.62/hr, ~1 hour) and
  `LINK_JOBS=8`.
- Ubuntu 24.04, toolchain via cloud-init (`user_data.sh`).
- SSH only, locked to your current public IP at apply time; static IP.

## Use

```bash
az login                                                  # once
export ARM_SUBSCRIPTION_ID=$(az account show --query id -o tsv)
terraform init && terraform apply

./server.sh start|stop|status|ip|ssh   # stop = DEALLOCATE (billing stops,
                                       # disk + ccache + llvm checkout persist)

./remote-build-clang.sh    # clone/update the fork's std-embed branch on the
                           # server, build clang+lld, SMOKE-TEST std::embed
                           # (compiles + runs examples/std/basic with the new
                           # clang++), fetch dist/clang-std-embed-*.tar.xz

./sync-to-ctbrowser.sh     # unpack newest archive into
                           # ../compile-time-browser/tools/clang-std-embed
                           # (adds it to that repo's .gitignore — binaries
                           # never get committed)

terraform destroy          # tear it all down
```

Iteration loop for std::embed development: edit in the llvm fork (locally in
`~/llvm-toolchain` or straight on the server in `~/llvm-project`), push to
`std-embed`, re-run `./remote-build-clang.sh` — ccache turns the rebuild into
minutes unless headers changed broadly. Regenerate the patch afterwards
(`git format-patch -1 HEAD --stdout > patches/0002-...patch`) so the repo's
patches stay the source of truth for CI.

## Conventions & secrets

Same rules as the sibling ctbrowser stack: `.terraform.lock.hcl` is committed;
`*.tfstate*`, `*.tfvars`, `tfplan`, `backend.hcl` are gitignored (state embeds
your detected home-IP CIDR); credentials only ever come from `az login` +
`ARM_SUBSCRIPTION_ID`; the VM is SSH-key-only with Trusted Launch enabled.
