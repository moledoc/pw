#!/usr/bin/python3
# NOTE: after downloading this script, update the path to python
# For A-Shell: #!/private/var/containers/Bundle/Application/.../python3
import subprocess

def vertical_print(lst):
	for e in lst:
		print(e)
	print("------------------------------")

def main():
	with open("./vault.contents", "r") as f:
		contents = f.read()
	lines = [line for line in contents.split("\n") if len(line) > 0]
	orig_domains = [line.split(" ")[2] for line in lines]

	domains = orig_domains.copy()
	domain = ""
	while True:
		vertical_print(domains)
		matches = []
		search_term = input("Search: ")
		# allow resetting the search
		if search_term == "-1":
			domains = orig_domains.copy()
			continue
		matches = [domain for domain in domains if search_term in domain]
		domains = matches
		if len(matches) == 1:
			domain = matches[0]
			break

	selected_line = [line for line in lines if line.split(" ")[2] == domain][0]
	sline_elems = selected_line.split(" ")

	salt = sline_elems[0]
	pepper = sline_elems[1]
	domain = sline_elems[2]
	extra = [e.replace("\"", "") for e in sline_elems[3:]]

	cmd = ["pw", "-k", "./.pw.key", "-s", f"{salt}", "-p", f"{pepper}", *extra, f"{domain}"] # NOTE: might need to change path to key file (-k flag)
	p1 = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True)
	p1.wait()
	p2 = subprocess.Popen(["pbcopy"], stdin=p1.stdout, text=True)
	p1.stdout.close()  # Allow p1 to receive a SIGPIPE if p2 exits.
	p2.wait()


if __name__ == "__main__":
    main()