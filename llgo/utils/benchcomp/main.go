package main

import (
	"bufio"
	"debug/elf"
	"debug/macho"
	"encoding/binary"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
)

func symsizes(path string) map[string]float64 {
	m := make(map[string]float64)
	f, err := elf.Open(path)
	if err != nil {
		panic(err.Error())
	}
	syms, err := f.Symbols()
	if err != nil {
		panic(err.Error())
	}
	for _, sym := range syms {
		if sym.Section < elf.SectionIndex(len(f.Sections)) && strings.HasPrefix(f.Sections[sym.Section].Name, ".text") {
			m[sym.Name] = float64(sym.Size)
		}
	}
	return m
}

type bySectionThenOffset []macho.Symbol

func (syms bySectionThenOffset) Len() int {
	return len(syms)
}

func (syms bySectionThenOffset) Less(i, j int) bool {
	if syms[i].Sect < syms[j].Sect {
		return true
	}
	if syms[i].Sect > syms[j].Sect {
		return false
	}
	return syms[i].Value < syms[j].Value
}

func (syms bySectionThenOffset) Swap(i, j int) {
	syms[i], syms[j] = syms[j], syms[i]
}

func macho_symsizes(path string) map[string]float64 {
	m := make(map[string]float64)
	f, err := macho.Open(path)
	if err != nil {
		panic(err.Error())
	}
	syms := make([]macho.Symbol, len(f.Symtab.Syms))
	copy(syms, f.Symtab.Syms)
	sort.Sort(bySectionThenOffset(syms))
	for i, sym := range syms {
		if sym.Sect == 0 {
			continue
		}
		var nextOffset uint64
		if i == len(syms)-1 || syms[i+1].Sect != sym.Sect {
			nextOffset = f.Sections[sym.Sect-1].Size
		} else {
			nextOffset = syms[i+1].Value
		}
		m[sym.Name] = float64(nextOffset - sym.Value)
	}
	return m
}

func benchnums(path, stat string) map[string]float64 {
	m := make(map[string]float64)

	fh, err := os.Open(path)
	if err != nil {
		panic(err.Error())
	}

	scanner := bufio.NewScanner(fh)
	for scanner.Scan() {
		elems := strings.Split(scanner.Text(), "\t")
		if !strings.HasPrefix(elems[0], "Benchmark") || len(elems) < 3 {
			continue
		}
		var s string
		for _, elem := range elems[2:] {
			selems := strings.Split(strings.TrimSpace(elem), " ")
			if selems[1] == stat {
				s = selems[0]
			}
		}
		if s != "" {
			ns, err := strconv.ParseFloat(s, 64)
			if err != nil {
				panic(scanner.Text() + " ---- " + err.Error())
			}
			m[elems[0]] = ns
		}
	}

	if err := scanner.Err(); err != nil {
		panic(err)
	}

	return m
}

func ninja_logs(path string) map[string]float64 {
	m := make(map[string]float64)

	fh, err := os.Open(path)
	if err != nil {
		panic(err.Error())
	}

	scanner := bufio.NewScanner(fh)
	for scanner.Scan() {
		elems := strings.Split(scanner.Text(), "\t")
		if len(elems) < 4 {
			continue
		}
		begin, err := strconv.ParseInt(elems[0], 10, 64)
		if err != nil {
			continue
		}
		end, err := strconv.ParseInt(elems[1], 10, 64)
		if err != nil {
			panic(err.Error())
		}
		m[elems[3]] = float64(end - begin)
	}

	return m
}

func filesizes(root string) map[string]float64 {
	m := make(map[string]float64)

	err := filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if info.Mode().IsRegular() {
			m[path[len(root):]] = float64(info.Size())
		}
		return nil
	})
	if err != nil {
		panic(err.Error())
	}

	return m
}

type reloc struct {
	offset, symno uint64
}

func read_relocs(data []byte, class elf.Class, t elf.SectionType, byteorder binary.ByteOrder) []reloc {
	var rel_size int
	if t == elf.SHT_RELA {
		rel_size = 12
	} else {
		rel_size = 8
	}
	if len(data)%rel_size != 0 {
		panic("huh")
	}

	rel_info_offset := 4
	word := func(data []byte) uint64 {
		return uint64(byteorder.Uint32(data))
	}
	var rel_symno_shift uint = 8
	if class == elf.ELFCLASS64 {
		rel_size = rel_size * 2
		rel_info_offset = rel_info_offset * 2
		rel_symno_shift = 32
		word = func(data []byte) uint64 {
			return byteorder.Uint64(data)
		}
	}

	var relocs []reloc
	for i := 0; i < len(data); i = i + rel_size {
		var r reloc
		r.offset = word(data[i:])
		r.symno = word(data[i+rel_info_offset:]) >> rel_symno_shift
		relocs = append(relocs, r)
	}
	return relocs
}

func sym_reloc_sizes(root string) map[string]float64 {
	m := make(map[string]float64)

	err := filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if !info.Mode().IsRegular() {
			return nil
		}
		f, err := elf.Open(path)
		if err != nil || f.Type != elf.ET_REL {
			return nil
		}
		rels := make([][]reloc, len(f.Sections))
		for _, s := range f.Sections {
			if s.Type == elf.SHT_REL || s.Type == elf.SHT_RELA {
				relsec_data, err := s.Data()
				if err != nil {
					panic(err.Error())
				}
				rels[s.Info] = read_relocs(relsec_data, f.Class, s.Type, f.ByteOrder)
			}
		}
		syms, err := f.Symbols()
		if err != nil {
			return nil
		}
		for _, sym := range syms {
			if sym.Section < elf.SectionIndex(len(f.Sections)) && strings.HasPrefix(f.Sections[sym.Section].Name, ".text") {
				name := sym.Name
				if sym.Size != 0 {
					for _, r := range rels[sym.Section] {
						if r.offset >= sym.Value && r.offset < sym.Value+sym.Size {
							name += "," + syms[r.symno-1].Name
						}
					}
				}
				m[name] = float64(sym.Size)
			}
		}

		return nil
	})
	if err != nil {
		panic(err.Error())
	}

	return m
}

func main() {
	var cmp func(string) map[string]float64
	switch os.Args[1] {
	case "symsizes":
		cmp = symsizes

	case "sym_reloc_sizes":
		cmp = sym_reloc_sizes

	case "macho_symsizes":
		cmp = macho_symsizes

	case "benchns":
		cmp = func(path string) map[string]float64 {
			return benchnums(path, "ns/op")
		}

	case "benchallocs":
		cmp = func(path string) map[string]float64 {
			return benchnums(path, "allocs/op")
		}

	case "ninja_logs":
		cmp = ninja_logs

	case "filesizes":
		cmp = filesizes
	}

	syms1 := cmp(os.Args[2])
	syms2 := cmp(os.Args[3])

	for n, z1 := range syms1 {
		if z2, ok := syms2[n]; ok && z2 != 0 {
			fmt.Printf("%s %f %f %f\n", n, z1, z2, z1/z2)
		}
	}
}
