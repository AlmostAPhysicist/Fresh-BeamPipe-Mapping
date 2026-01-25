import subprocess

try:
    # This command will fail (assuming a non-existent command)
    # output = subprocess.run(["ls", "-la"], check=True, capture_output=True, text=True)
    # print("Here is the output:\n", output.stdout)

    output = subprocess.run("echo 'Hello World' > testfile.txt", shell=True, check=True, capture_output=True, text=True)
    file_content = subprocess.run("cat testfile.txt", shell=True, check=True, capture_output=True, text=True)
    print("File content:\n", file_content.stdout)
except subprocess.CalledProcessError as e:
    print(f"Command failed with exit code {e.returncode}")
    print("Error output:", e.stderr)
